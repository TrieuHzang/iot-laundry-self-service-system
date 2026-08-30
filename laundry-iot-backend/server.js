require("dotenv").config();

const express = require("express");
const cors = require("cors");
const path = require("path");
const PayOS = require("@payos/node");

const app = express();
const PORT = process.env.PORT || 3000;

const payos = new PayOS(
  process.env.PAYOS_CLIENT_ID,
  process.env.PAYOS_API_KEY,
  process.env.PAYOS_CHECKSUM_KEY
);

app.use(
  cors({
    origin: "*",
    methods: ["GET", "POST", "PUT", "DELETE", "OPTIONS"],
    allowedHeaders: ["Content-Type", "Authorization"],
  })
);

app.use(express.json());

const orderStore = new Map();

function getCommandByMachineId(machineId) {
  if (machineId === "wash_01") return "WASH";
  if (machineId === "dry_01") return "DRY";
  if (machineId === "wash_dry_01") return "WASHDRY";

  return "";
}

async function sendCommandToBlynk(machineId) {
  const command = getCommandByMachineId(machineId);

  if (!command) {
    console.log("MachineId không hợp lệ:", machineId);
    return false;
  }

  const token = process.env.BLYNK_TOKEN;

  if (!token) {
    console.log("Chưa cấu hình BLYNK_TOKEN trên Render");
    return false;
  }

  try {
    const pin = process.env.BLYNK_PIN || "V0";

const url =
  `https://blynk.cloud/external/api/update?token=${token}&${pin}=${encodeURIComponent(command)}`;

    const response = await fetch(url);

    if (!response.ok) {
      throw new Error(`Blynk HTTP ${response.status}`);
    }

    console.log(`Đã gửi lệnh sang Blynk: ${command}`);
    return true;
  } catch (error) {
    console.log("Không gửi được lệnh sang Blynk:", error.message);
    return false;
  }
}

async function markOrderPaidAndSendCommand(orderCode, transactionId = null) {
  const order = orderStore.get(String(orderCode));

  if (!order) {
    console.log("Không tìm thấy đơn hàng:", orderCode);
    return null;
  }

  order.status = "PAID";
  order.paidAt = order.paidAt || new Date().toISOString();

  if (transactionId) {
    order.transactionId = transactionId;
  }

  if (!order.commandSent) {
    const sent = await sendCommandToBlynk(order.machineId);
    order.commandSent = sent;
    order.command = getCommandByMachineId(order.machineId);
    order.commandSentAt = sent ? new Date().toISOString() : null;
  } else {
    console.log(`Đơn #${orderCode} đã gửi lệnh rồi, không gửi lại`);
  }

  orderStore.set(String(orderCode), order);
  return order;
}

app.get("/api/health", (req, res) => {
  res.json({
    status: "OK",
    message: "Laundry IoT Backend đang chạy",
    timestamp: new Date().toISOString(),
  });
});

app.post("/api/create-payment-link", async (req, res) => {
  try {
    const { machineId, machineName, service, amount } = req.body;

    if (!machineId || !amount) {
      return res.status(400).json({
        error: "Thiếu machineId hoặc amount",
      });
    }

    const command = getCommandByMachineId(machineId);

    if (!command) {
      return res.status(400).json({
        error: "machineId không hợp lệ",
      });
    }

    const orderCode = Number(String(Date.now()).slice(-8));

    const frontendUrl =
      process.env.FRONTEND_URL ||
      "https://laundry-iot-web.kiananh123.workers.dev";

    const paymentData = {
      orderCode,
      amount: Math.round(amount),
      description: `Laundry-${machineId}`.slice(0, 25),
      returnUrl: `${frontendUrl}/?payment=success&orderCode=${orderCode}`,
      cancelUrl: `${frontendUrl}/?payment=cancel&orderCode=${orderCode}`,
      items: [
        {
          name: `${machineName} - ${service}`.slice(0, 100),
          quantity: 1,
          price: Math.round(amount),
        },
      ],
    };

    const paymentLink = await payos.createPaymentLink(paymentData);

    orderStore.set(String(orderCode), {
      status: "PENDING",
      machineId,
      machineName,
      service,
      amount,
      command,
      commandSent: false,
      createdAt: new Date().toISOString(),
      paymentLinkId: paymentLink.paymentLinkId,
    });

    console.log(
      `[PayOS] Tạo đơn #${orderCode} - ${machineId} - ${command} - ${amount}đ`
    );

    res.json({
      success: true,
      checkoutUrl: paymentLink.checkoutUrl,
      orderCode,
      paymentLinkId: paymentLink.paymentLinkId,
      qrCode: paymentLink.qrCode,
    });
  } catch (error) {
    console.error("[PayOS] Lỗi tạo payment link:", error);

    res.status(500).json({
      error: "Không thể tạo link thanh toán",
      detail: error.message,
    });
  }
});

app.get("/api/payment-status/:orderCode", async (req, res) => {
  try {
    const { orderCode } = req.params;

    const localOrder = orderStore.get(String(orderCode));

    if (!localOrder) {
      return res.status(404).json({
        error: "Không tìm thấy đơn hàng",
      });
    }

    if (localOrder.status === "PAID") {
      return res.json({
        status: "PAID",
        orderCode,
        ...localOrder,
      });
    }

    try {
      const paymentInfo = await payos.getPaymentLinkInformation(orderCode);

      if (paymentInfo.status === "PAID") {
        const paidOrder = await markOrderPaidAndSendCommand(orderCode);

        return res.json({
          status: "PAID",
          orderCode,
          ...paidOrder,
        });
      }

      if (paymentInfo.status === "CANCELLED") {
        localOrder.status = "CANCELLED";
        orderStore.set(String(orderCode), localOrder);
      }

      return res.json({
        status: localOrder.status,
        orderCode,
        ...localOrder,
      });
    } catch (error) {
      console.log("[PayOS] Chưa lấy được trạng thái payOS:", error.message);

      return res.json({
        status: localOrder.status,
        orderCode,
        ...localOrder,
      });
    }
  } catch (error) {
    console.error("[PayOS] Lỗi kiểm tra trạng thái:", error);

    res.status(500).json({
      error: "Lỗi kiểm tra trạng thái thanh toán",
    });
  }
});

app.post("/api/cancel-payment/:orderCode", async (req, res) => {
  try {
    const { orderCode } = req.params;

    await payos.cancelPaymentLink(orderCode, "Người dùng huỷ đơn hàng");

    const order = orderStore.get(String(orderCode));

    if (order) {
      order.status = "CANCELLED";
      orderStore.set(String(orderCode), order);
    }

    res.json({
      success: true,
    });
  } catch (error) {
    console.error("[PayOS] Lỗi huỷ đơn hàng:", error);

    res.status(500).json({
      error: "Không thể huỷ đơn hàng",
    });
  }
});

app.post("/api/webhook", async (req, res) => {
  try {
    const webhookData = payos.verifyPaymentWebhookData(req.body);
    const { orderCode, code, desc, data } = webhookData;

    console.log(`[Webhook] Đơn #${orderCode}: ${code} - ${desc}`);

    if (code === "00") {
      await markOrderPaidAndSendCommand(orderCode, data?.reference);
    }

    res.json({
      success: true,
    });
  } catch (error) {
    console.error("[Webhook] Lỗi:", error);

    res.status(400).json({
      error: "Webhook không hợp lệ",
    });
  }
});

// Admin cấp quyền thủ công khi khách trả tiền mặt hoặc payOS lỗi
app.post("/api/admin/manual-command", async (req, res) => {
  try {
    const { machineId } = req.body;

    const command = getCommandByMachineId(machineId);

    if (!command) {
      return res.status(400).json({
        error: "machineId không hợp lệ",
      });
    }

    const sent = await sendCommandToBlynk(machineId);

    res.json({
      success: sent,
      machineId,
      command,
      message: sent
        ? `Đã gửi lệnh ${command} sang Blynk`
        : "Gửi lệnh Blynk thất bại",
    });
  } catch (error) {
    res.status(500).json({
      error: "Lỗi gửi lệnh thủ công",
      detail: error.message,
    });
  }
});

// Frontend để cuối cùng
const publicDir = path.join(__dirname, "public");

app.use(express.static(publicDir));

app.get("*", (req, res) => {
  res.sendFile(path.join(publicDir, "index.html"));
});

app.listen(PORT, "0.0.0.0", () => {
  console.log(`Server running on port ${PORT}`);
  console.log(`Backend URL: https://laundry-iot-backend.onrender.com`);
  console.log("Sẵn sàng nhận thanh toán và gửi lệnh qua Blynk");
});
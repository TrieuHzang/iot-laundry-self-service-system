/**
 * tunnel.js — Khởi động server + tạo public URL qua Cloudflare Tunnel
 * Chạy: node tunnel.js
 *
 * Điện thoại bất kỳ mạng nào cũng truy cập được qua URL được in ra.
 */

// Bật LOG chi tiết từ cloudflared
process.env.VERBOSE = "true";

require("dotenv").config();
const { execSync } = require("child_process");
const { Tunnel, bin, install } = require("cloudflared");
const path = require("path");
const fs = require("fs");
const https = require("https");

const PORT = process.env.PORT || 3000;
const KV_APP_KEY = "nkkmgtz3"; // Key-Value App Key cố định

// Hàm gửi URL backend lên Key-Value store trung gian
function updateBackendUrl(publicUrl) {
  return new Promise((resolve, reject) => {
    const hex = Buffer.from(publicUrl).toString("hex");
    const url = `https://keyvalue.immanuel.co/api/KeyVal/UpdateValue/${KV_APP_KEY}/backend_url/${hex}`;

    const req = https.request(
      url,
      {
        method: "POST",
        headers: {
          "Content-Length": "0",
        },
      },
      (res) => {
        let data = "";
        res.on("data", (chunk) => (data += chunk));
        res.on("end", () => {
          if (data.trim() === "true") {
            resolve();
          } else {
            reject(new Error("Phản hồi từ KV store không hợp lệ: " + data));
          }
        });
      }
    );

    req.on("error", (err) => {
      reject(err);
    });

    req.end();
  });
}

async function main() {
  // 1. Build frontend trước
  console.log("🔨 Đang build frontend React...");
  const frontendDir = path.join(__dirname, "../laundry-iot-web");

  if (!fs.existsSync(frontendDir)) {
    console.error("❌ Không tìm thấy thư mục laundry-iot-web!");
    process.exit(1);
  }

  try {
    execSync("npm run build", { cwd: frontendDir, stdio: "inherit" });
    console.log("✅ Build frontend thành công!\n");
  } catch (err) {
    console.error("❌ Build frontend thất bại:", err.message);
    process.exit(1);
  }

  // 2. Đảm bảo cloudflared binary đã được cài đặt
  if (!fs.existsSync(bin)) {
    console.log("📥 Đang cài đặt binary cloudflared...");
    await install(bin);
    console.log("✅ Cài đặt cloudflared thành công!\n");
  }

  // 3. Tạo Cloudflare Tunnel
  console.log("🌐 Đang tạo public URL qua Cloudflare Tunnel...");
  const tunnel = Tunnel.quick("http://localhost:" + PORT);

  // Lắng nghe sự kiện exit của process tunnel
  tunnel.on("exit", (code, signal) => {
    console.log(`\n🛑 [Tunnel] Tiến trình cloudflared đã thoát với code ${code}, signal ${signal}`);
    process.exit(1);
  });

  let publicUrl;
  try {
    publicUrl = await new Promise((resolve, reject) => {
      let resolved = false;
      tunnel.once("url", (url) => {
        resolved = true;
        resolve(url);
      });
      tunnel.once("error", (err) => {
        if (!resolved) {
          reject(err);
        }
      });
    });
  } catch (err) {
    console.error("❌ Lỗi tạo tunnel:", err.message);
    process.exit(1);
  }

  console.log(`\n🎉 Public URL: ${publicUrl}`);

  // 4. Cập nhật URL động lên Key-Value store trung gian
  console.log("📤 Đang cập nhật URL lên Key-Value Routing...");
  try {
    await updateBackendUrl(publicUrl);
    console.log("✅ Cập nhật URL định tuyến thành công!");
  } catch (err) {
    console.warn("⚠️ Cảnh báo: Không thể cập nhật URL định tuyến:", err.message);
    console.warn("Lưu ý: Có thể thiết bị di động không kết nối được nếu KV Store lỗi.");
  }

  console.log(`📱 Điện thoại mở link cố định của bạn (sau khi deploy Pages) hoặc dùng link trực tiếp này: ${publicUrl}\n`);

  // Ghi PUBLIC_URL vào env để server dùng cho returnUrl/cancelUrl
  process.env.PUBLIC_URL = publicUrl;

  // 5. Khởi động Express server
  require("./server.js");

  // Xử lý tunnel error
  tunnel.on("error", (err) => {
    console.error("[Tunnel] Lỗi:", err.message);
  });

  // Bắt Ctrl+C để dọn dẹp tunnel
  process.on("SIGINT", () => {
    console.log("\n🛑 Đang tắt tunnel...");
    tunnel.stop();
    process.exit(0);
  });
}

main().catch((err) => {
  console.error("❌ Lỗi khởi động:", err.message);
  process.exit(1);
});

import { useState, useEffect, useRef, useCallback } from "react";
import "./App.css";

// AppKey của Key-Value store lưu link backend động
const BACKEND_URL = "https://laundry-iot-backend.onrender.com";

function App() {
  const machines = [
    {
      id: "wash_01",
      name: "Máy giặt 01",
      service: "Giặt",
      time: 30,
      price: 2000,
      status: "Rảnh",
      icon: "🧺",
    },
    {
      id: "dry_01",
      name: "Máy sấy 01",
      service: "Sấy",
      time: 40,
      price: 2000,
      status: "Rảnh",
      icon: "☀️",
    },
    {
      id: "wash_dry_01",
      name: "Giặt + Sấy 01",
      service: "Giặt + Sấy",
      time: 70,
      price: 2000,
      status: "Rảnh",
      icon: "🧼",
    },
  ];
const backendUrl = BACKEND_URL;
const [selectedMachineId, setSelectedMachineId] = useState("wash_01");
  const [paymentState, setPaymentState] = useState("idle"); // idle | loading | pending | paid | cancelled | error
  const [orderCode, setOrderCode] = useState(null);
  const [checkoutUrl, setCheckoutUrl] = useState(null);
  const [qrCode, setQrCode] = useState(null);
  const [errorMsg, setErrorMsg] = useState("");
  const [showModal, setShowModal] = useState(false);
  const [pollingCount, setPollingCount] = useState(0);
  const pollingRef = useRef(null);
  const popupRef = useRef(null);

  const selectedMachine = machines.find((m) => m.id === selectedMachineId);

  // ── Dừng polling ────────────────────────────────────────────────────────────
  const stopPolling = useCallback(() => {
    if (pollingRef.current) {
      clearInterval(pollingRef.current);
      pollingRef.current = null;
    }
  }, []);

  // ── Polling kiểm tra trạng thái thanh toán ──────────────────────────────────
  const startPolling = useCallback(
    (code) => {
      stopPolling();
      pollingRef.current = setInterval(async () => {
        try {
          const res = await fetch(`${backendUrl}/api/payment-status/${code}`);
          const data = await res.json();
          setPollingCount((c) => c + 1);

          if (data.status === "PAID") {
            stopPolling();
            setPaymentState("paid");
            if (popupRef.current) popupRef.current.close();
          } else if (data.status === "CANCELLED") {
            stopPolling();
            setPaymentState("cancelled");
            if (popupRef.current) popupRef.current.close();
          }
        } catch {
          // Bỏ qua lỗi network tạm thời khi polling
        }
      }, 3000);
    },
    [backendUrl, stopPolling]
  );

  // ── Tạo link thanh toán ─────────────────────────────────────────────────────
  const handlePay = async () => {
    setPaymentState("loading");
    setErrorMsg("");

    try {
      const res = await fetch(`${backendUrl}/api/create-payment-link`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          machineId: selectedMachine.id,
          machineName: selectedMachine.name,
          service: selectedMachine.service,
          amount: selectedMachine.price,
        }),
      });

      const data = await res.json();

      if (!res.ok || !data.checkoutUrl) {
        throw new Error(data.error || "Không thể tạo link thanh toán");
      }

      setOrderCode(data.orderCode);
      setCheckoutUrl(data.checkoutUrl);
      setQrCode(data.qrCode);
      setPaymentState("pending");
      setShowModal(true);

      // Mở tab payOS
      popupRef.current = window.open(data.checkoutUrl, "_blank");

      // Bắt đầu polling
      startPolling(data.orderCode);
    } catch (err) {
      setPaymentState("error");
      setErrorMsg(err.message);
    }
  };

  // ── Huỷ đơn hàng ────────────────────────────────────────────────────────────
  const handleCancel = async () => {
    stopPolling();
    if (orderCode) {
      try {
        await fetch(`${backendUrl}/api/cancel-payment/${orderCode}`, {
          method: "POST",
        });
      } catch {}
    }
    if (popupRef.current) popupRef.current.close();
    setPaymentState("idle");
    setShowModal(false);
    setOrderCode(null);
    setCheckoutUrl(null);
    setQrCode(null);
    setPollingCount(0);
  };

  // ── Chọn máy mới → reset ─────────────────────────────────────────────────────
  const handleSelectMachine = (id) => {
    if (paymentState === "pending") return; // Không đổi máy khi đang thanh toán
    setSelectedMachineId(id);
    setPaymentState("idle");
    setOrderCode(null);
    setCheckoutUrl(null);
    setQrCode(null);
    setErrorMsg("");
    setShowModal(false);
    stopPolling();
  };

  // ── Bắt đầu lại ──────────────────────────────────────────────────────────────
  const handleReset = () => {
    setPaymentState("idle");
    setOrderCode(null);
    setCheckoutUrl(null);
    setQrCode(null);
    setErrorMsg("");
    setShowModal(false);
    setPollingCount(0);
    stopPolling();
  };

  // ── Cleanup khi unmount ───────────────────────────────────────────────────────
  useEffect(() => () => stopPolling(), [stopPolling]);

  // ── Step indicator ───────────────────────────────────────────────────────────
  const currentStep =
    paymentState === "idle" || paymentState === "loading"
      ? 1
      : paymentState === "pending"
      ? 2
      : paymentState === "paid"
      ? 4
      : 2;

  return (
    <div className="page">
      {/* ── Header ──────────────────────────────────────────────────────────── */}
      <header className="header">
        <div className="logo-group">
          <div className="logo-icon">🌀</div>
          <div>
            <h2 className="logo-text">Laundry 24/7</h2>
            <p className="logo-sub">Giặt sạch · Sấy khô · Tiện lợi</p>
          </div>
        </div>
        <nav className="nav">
          <span className="nav-item active">Trang chủ</span>
          <span className="nav-item">Lịch sử</span>
          <span className="nav-item">Hướng dẫn</span>
        </nav>
      </header>

      {/* ── Layout ──────────────────────────────────────────────────────────── */}
      <div className="layout">
        {/* Sidebar */}
        <aside className="sidebar">
          <h3 className="sidebar-title">Chọn máy</h3>
          {machines.map((m) => (
            <div
              key={m.id}
              onClick={() => handleSelectMachine(m.id)}
              className={`machine-card ${selectedMachineId === m.id ? "selected" : ""} ${paymentState === "pending" ? "locked" : ""}`}
            >
              <div className="machine-icon">{m.icon}</div>
              <div className="machine-info">
                <h4>{m.name}</h4>
                <p>{m.service}</p>
              </div>
              <span className="status-badge">{m.status}</span>
            </div>
          ))}
        </aside>

        {/* Main */}
        <main className="main">
          {/* Title + step */}
          <div className="title-row">
            <h1>{selectedMachine.name}</h1>
            <span className="ready-badge">● {selectedMachine.status}</span>
          </div>

          {/* Steps */}
          <div className="steps">
            {["Chọn máy", "Thanh toán", "Xử lý", "Hoàn thành"].map(
              (step, i) => (
                <div
                  key={i}
                  className={`step ${currentStep === i + 1 ? "step-active" : ""} ${currentStep > i + 1 ? "step-done" : ""}`}
                >
                  <div className="step-num">{currentStep > i + 1 ? "✓" : i + 1}</div>
                  <span>{step}</span>
                </div>
              )
            )}
          </div>

          {/* Service info */}
          <section className="info-box">
            <h3>Thông tin dịch vụ</h3>
            <div className="info-grid">
              <div className="info-item">
                <span className="info-label">Dịch vụ</span>
                <span className="info-value">{selectedMachine.service}</span>
              </div>
              <div className="info-item">
                <span className="info-label">Thời gian</span>
                <span className="info-value">{selectedMachine.time} phút</span>
              </div>
              <div className="info-item">
                <span className="info-label">Giá tiền</span>
                <span className="info-value price">
                  {selectedMachine.price.toLocaleString("vi-VN")}đ
                </span>
              </div>
            </div>
          </section>

          {/* Payment section */}
          <section className="payment-section">
            <h3>Thanh toán qua payOS</h3>

            {paymentState === "idle" && (
              <div className="payment-idle">
                <div className="payos-logos">
                  <div className="bank-chips">
                    <span className="chip">MB Bank</span>
                    <span className="chip">VietcomBank</span>
                    <span className="chip">Techcombank</span>
                    <span className="chip">BIDV</span>
                    <span className="chip">+ 30 ngân hàng</span>
                  </div>
                  <p className="payos-note">
                    Thanh toán an toàn qua hệ thống payOS — hỗ trợ chuyển
                    khoản nhanh 24/7
                  </p>
                </div>
                <button className="pay-btn" onClick={handlePay}>
                  <span className="pay-btn-icon">💳</span>
                  Thanh toán{" "}
                  {selectedMachine.price.toLocaleString("vi-VN")}đ ngay
                </button>
              </div>
            )}

            {paymentState === "loading" && (
              <div className="state-box loading">
                <div className="spinner" />
                <p>Đang tạo link thanh toán...</p>
              </div>
            )}

            {paymentState === "pending" && (
              <div className="state-box pending">
                <div className="pending-header">
                  <div className="pulse-dot" />
                  <span>Đang chờ thanh toán</span>
                </div>
                <p className="pending-sub">
                  Trang thanh toán payOS đã mở trong tab mới.
                  <br />
                  Hệ thống tự động xác nhận sau khi bạn thanh toán.
                </p>
                <div className="order-code-box">
                  <span className="order-label">Mã đơn hàng</span>
                  <span className="order-code">#{orderCode}</span>
                </div>
                <div className="polling-info">
                  <div className="polling-dot" />
                  <span>Đang kiểm tra... ({pollingCount} lần)</span>
                </div>
                <div className="pending-actions">
                  <button
                    className="reopen-btn"
                    onClick={() => {
                      if (checkoutUrl) window.open(checkoutUrl, "_blank");
                    }}
                  >
                    🔗 Mở lại trang thanh toán
                  </button>
                  <button className="cancel-btn" onClick={handleCancel}>
                    Huỷ đơn hàng
                  </button>
                </div>
              </div>
            )}

            {paymentState === "paid" && (
              <div className="state-box success">
                <div className="success-icon">✅</div>
                <h2>Thanh toán thành công!</h2>
                <p>
                  Máy <strong>{selectedMachine.name}</strong> sẽ khởi động
                  trong giây lát.
                </p>
                <div className="order-code-box success-code">
                  <span className="order-label">Mã đơn hàng</span>
                  <span className="order-code">#{orderCode}</span>
                </div>
                <div className="machine-running">
                  <span className="running-icon">⚙️</span>
                  <span>Máy đang khởi động...</span>
                </div>
                <button className="reset-btn" onClick={handleReset}>
                  Đặt giặt mới
                </button>
              </div>
            )}

            {paymentState === "cancelled" && (
              <div className="state-box cancelled">
                <div className="cancel-icon">❌</div>
                <h3>Đơn hàng đã huỷ</h3>
                <p>Bạn có thể thực hiện thanh toán mới.</p>
                <button className="pay-btn" onClick={handleReset}>
                  Thử lại
                </button>
              </div>
            )}

            {paymentState === "error" && (
              <div className="state-box error">
                <div className="error-icon">⚠️</div>
                <h3>Có lỗi xảy ra</h3>
                <p className="error-msg">{errorMsg}</p>
                <button className="pay-btn" onClick={handleReset}>
                  Thử lại
                </button>
              </div>
            )}
          </section>
        </main>

        {/* Order sidebar */}
        <aside className="order-sidebar">
          <div className="order-box">
            <h3>Thông tin đơn hàng</h3>
            <div className="order-row">
              <span>Máy</span>
              <span>{selectedMachine.name}</span>
            </div>
            <div className="order-row">
              <span>Dịch vụ</span>
              <span>{selectedMachine.service}</span>
            </div>
            <div className="order-row">
              <span>Thời gian</span>
              <span>{selectedMachine.time} phút</span>
            </div>
            <hr className="order-divider" />
            <div className="order-total">
              <span>Tổng tiền</span>
              <span className="total-price">
                {selectedMachine.price.toLocaleString("vi-VN")}đ
              </span>
            </div>
            {orderCode && (
              <div className="order-code-small">
                Mã ĐH: <strong>#{orderCode}</strong>
              </div>
            )}
            {paymentState === "paid" && (
              <div className="paid-badge-box">✅ Đã thanh toán</div>
            )}
          </div>

          <div className="guide-box">
            <h3>Hướng dẫn</h3>
            <div className="guide-step">
              <span className="guide-num">1</span>
              <span>Chọn máy phù hợp</span>
            </div>
            <div className="guide-step">
              <span className="guide-num">2</span>
              <span>Bấm "Thanh toán ngay"</span>
            </div>
            <div className="guide-step">
              <span className="guide-num">3</span>
              <span>Quét mã QR hoặc chuyển khoản</span>
            </div>
            <div className="guide-step">
              <span className="guide-num">4</span>
              <span>Máy tự động khởi động</span>
            </div>
          </div>

          <div className="payos-badge">
            <span>🔒 Thanh toán bảo mật bởi</span>
            <strong> payOS</strong>
          </div>
        </aside>
      </div>
    </div>
  );
}

export default App;
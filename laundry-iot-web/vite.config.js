import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

// https://vite.dev/config/
export default defineConfig({
  plugins: [react()],
  build: {
    outDir: '../laundry-iot-backend/public',  // build thẳng vào thư mục backend
    emptyOutDir: true,
  },
  server: {
    host: true,   // expose trên tất cả network interfaces (0.0.0.0)
    port: 5173,   // cố định port
    proxy: {
      // Khi dev: forward /api calls sang backend port 3000
      '/api': 'http://localhost:3000',
    },
  },
})

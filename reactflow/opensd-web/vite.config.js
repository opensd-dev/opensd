import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'
import { solverServerPlugin } from './solverServer.js'

// https://vite.dev/config/
export default defineConfig({
  plugins: [react(), solverServerPlugin()],
  server: {
    host: '0.0.0.0',
    allowedHosts: true
  },
  preview: {
    host: '0.0.0.0',
    allowedHosts: true
  }
})

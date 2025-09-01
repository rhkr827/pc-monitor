/** @type {import('tailwindcss').Config} */
export default {
  content: [
    "./index.html",
    "./src/**/*.{js,ts,jsx,tsx}",
  ],
  darkMode: 'class',
  theme: {
    extend: {
      colors: {
        'bg-primary': '#1a1a1a',
        'bg-secondary': '#2d2d2d',
        'text-primary': '#ffffff',
        'text-secondary': '#b3b3b3',
        'cpu-low': '#22c55e',
        'cpu-medium': '#eab308',
        'cpu-high': '#ef4444',
        'memory-safe': '#06b6d4',
        'memory-warning': '#f59e0b',
        'memory-critical': '#dc2626',
      },
      animation: {
        'pulse-slow': 'pulse 3s cubic-bezier(0.4, 0, 0.6, 1) infinite',
      }
    },
  },
  plugins: [],
}
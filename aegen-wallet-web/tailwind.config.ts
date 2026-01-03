import type { Config } from "tailwindcss";

const config: Config = {
    content: [
        "./pages/**/*.{js,ts,jsx,tsx,mdx}",
        "./components/**/*.{js,ts,jsx,tsx,mdx}",
        "./app/**/*.{js,ts,jsx,tsx,mdx}",
    ],
    theme: {
        extend: {
            colors: {
                background: "var(--background)",
                foreground: "var(--foreground)",
                primary: {
                    DEFAULT: "#8b5cf6", // Violet
                    foreground: "#ffffff",
                },
                secondary: {
                    DEFAULT: "#06b6d4", // Cyan
                    foreground: "#ffffff",
                },
                card: {
                    DEFAULT: "rgba(30, 41, 59, 0.7)", // Glassy dark
                    foreground: "#ffffff",
                }
            },
            backgroundImage: {
                'gradient-radial': 'radial-gradient(var(--tw-gradient-stops))',
                'gradient-conic': 'conic-gradient(from 180deg at 50% 50%, var(--tw-gradient-stops))',
                'hero-glow': 'radial-gradient(circle at center, rgba(139, 92, 246, 0.15) 0%, rgba(6, 182, 212, 0.05) 50%, transparent 100%)',
            },
            backdropBlur: {
                xs: '2px',
            }
        },
    },
    plugins: [],
};
export default config;

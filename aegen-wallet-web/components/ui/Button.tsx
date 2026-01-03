import { ButtonHTMLAttributes, forwardRef } from 'react';
import { clsx, type ClassValue } from 'clsx';
import { twMerge } from 'tailwind-merge';
import { motion, HTMLMotionProps } from 'framer-motion';

function cn(...inputs: ClassValue[]) {
    return twMerge(clsx(inputs));
}

interface ButtonProps extends React.ButtonHTMLAttributes<HTMLButtonElement> {
    variant?: 'primary' | 'secondary' | 'ghost' | 'danger';
    isLoading?: boolean;
}

export const Button = forwardRef<HTMLButtonElement, ButtonProps>(
    ({ className, variant = 'primary', isLoading, children, ...props }, ref) => {
        const variants = {
            primary: "bg-gradient-to-r from-violet-600 to-cyan-500 text-white shadow-lg shadow-violet-900/20 hover:shadow-cyan-500/30 hover:scale-[1.02]",
            secondary: "bg-slate-800 border border-slate-700 text-slate-200 hover:bg-slate-700 hover:border-slate-600",
            ghost: "bg-transparent text-slate-400 hover:text-white hover:bg-white/5",
            danger: "bg-red-500/10 text-red-400 border border-red-500/20 hover:bg-red-500/20"
        };

        return (
            <motion.button
                ref={ref as any}
                whileTap={{ scale: 0.98 }}
                className={cn(
                    "relative inline-flex items-center justify-center rounded-xl px-6 py-3 text-sm font-semibold transition-all duration-200 disabled:opacity-50 disabled:cursor-not-allowed",
                    variants[variant],
                    className
                )}
                disabled={isLoading || props.disabled}
                {...(props as any)}
            >
                {isLoading && (
                    <svg className="animate-spin -ml-1 mr-3 h-4 w-4 text-current" xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24">
                        <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4"></circle>
                        <path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"></path>
                    </svg>
                )}
                {children}
            </motion.button>
        );
    }
);

Button.displayName = "Button";

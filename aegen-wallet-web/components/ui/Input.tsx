import { InputHTMLAttributes, forwardRef } from 'react';
import { clsx, type ClassValue } from 'clsx';
import { twMerge } from 'tailwind-merge';

function cn(...inputs: ClassValue[]) {
    return twMerge(clsx(inputs));
}

interface InputProps extends InputHTMLAttributes<HTMLInputElement> {
    label?: string;
    error?: string;
}

export const Input = forwardRef<HTMLInputElement, InputProps>(
    ({ className, label, error, ...props }, ref) => {
        return (
            <div className="space-y-2 w-full">
                {label && <label className="text-xs font-medium text-slate-400 uppercase tracking-wider ml-1">{label}</label>}
                <div className="relative group">
                    <div className="absolute -inset-0.5 bg-gradient-to-r from-violet-600 to-cyan-500 rounded-xl opacity-0 group-focus-within:opacity-50 transition duration-500 blur"></div>
                    <input
                        ref={ref}
                        className={cn(
                            "relative w-full bg-slate-900 border border-slate-700/50 rounded-xl px-4 py-3 text-slate-100 placeholder:text-slate-600 focus:outline-none focus:bg-slate-900/80 transition-all font-mono text-sm",
                            error ? "border-red-500/50 focus:border-red-500" : "focus:border-transparent",
                            className
                        )}
                        {...props}
                    />
                </div>
                {error && <p className="text-xs text-red-400 ml-1">{error}</p>}
            </div>
        );
    }
);

Input.displayName = "Input";

import { clsx, type ClassValue } from 'clsx';
import { twMerge } from 'tailwind-merge';

export function cn(...inputs: ClassValue[]) {
    return twMerge(clsx(inputs));
}

export function formatAddress(addr: string) {
    if (!addr) return '';
    return addr.substring(0, 10) + '...' + addr.substring(addr.length - 6);
}

export function copyToClipboard(text: string) {
    navigator.clipboard.writeText(text);
}

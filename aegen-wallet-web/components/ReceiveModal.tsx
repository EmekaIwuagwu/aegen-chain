import { motion, AnimatePresence } from 'framer-motion';
import { X, Copy, Share2 } from 'lucide-react';
import { copyToClipboard } from '../lib/utils';
import { useState } from 'react';
import { Button } from './ui/Button';
import { QRCodeSVG } from 'qrcode.react';

interface ReceiveModalProps {
    isOpen: boolean;
    onClose: () => void;
    address: string;
}

export function ReceiveModal({ isOpen, onClose, address }: ReceiveModalProps) {
    const [copied, setCopied] = useState(false);

    if (!isOpen) return null;

    const handleCopy = () => {
        copyToClipboard(address);
        setCopied(true);
        setTimeout(() => setCopied(false), 2000);
    };

    return (
        <AnimatePresence>
            <div className="fixed inset-0 z-50 flex items-center justify-center p-4">
                <motion.div
                    initial={{ opacity: 0 }} animate={{ opacity: 1 }} exit={{ opacity: 0 }}
                    onClick={onClose} className="absolute inset-0 bg-black/60 backdrop-blur-sm"
                />
                <motion.div
                    initial={{ opacity: 0, scale: 0.95, y: 20 }}
                    animate={{ opacity: 1, scale: 1, y: 0 }}
                    exit={{ opacity: 0, scale: 0.95, y: 20 }}
                    className="relative w-full max-w-sm bg-slate-900 border border-slate-700/50 rounded-3xl p-8 shadow-2xl z-10 text-center"
                >
                    <button onClick={onClose} className="absolute top-4 right-4 p-2 hover:bg-white/5 rounded-full transition text-slate-400 hover:text-white">
                        <X className="w-5 h-5" />
                    </button>

                    <h3 className="text-xl font-bold text-white mb-2">Receive Assets</h3>
                    <p className="text-sm text-slate-400 mb-8">Scan or copy address to receive funds</p>

                    <div className="flex justify-center mb-8">
                        <div className="bg-white p-4 rounded-2xl shadow-lg shadow-cyan-500/10 relative group">
                            <QRCodeSVG
                                value={address}
                                size={200}
                                fgColor="#0f172a"
                                bgColor="#ffffff"
                                level="Q"
                                includeMargin={false}
                            />
                            {/* Brand Overlay (Aegen Logo in center if desired, but keeping clean for reliability) */}
                        </div>
                    </div>

                    <div className="bg-slate-950/50 rounded-xl p-4 border border-slate-800 mb-6">
                        <div className="text-xs text-slate-500 uppercase mb-2 text-left">Your Address</div>
                        <div className="font-mono text-sm text-cyan-400 break-all text-left leading-relaxed">
                            {address}
                        </div>
                    </div>

                    <Button onClick={handleCopy} className={`w-full ${copied ? 'bg-emerald-500/20 text-emerald-400 border-emerald-500/30' : ''}`}>
                        <Copy className="w-4 h-4 mr-2" />
                        {copied ? 'Copied to Clipboard!' : 'Copy Address'}
                    </Button>
                </motion.div>
            </div>
        </AnimatePresence>
    );
}

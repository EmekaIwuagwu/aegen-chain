import { motion, AnimatePresence } from 'framer-motion';
import { X, AlertTriangle, Eye, EyeOff, Copy, Shield, Check } from 'lucide-react';
import { useState } from 'react';
import { Button } from './ui/Button';
import { copyToClipboard } from '../lib/utils';

interface PrivateKeyModalProps {
    isOpen: boolean;
    onClose: () => void;
    privateKey: string;
}

export function PrivateKeyModal({ isOpen, onClose, privateKey }: PrivateKeyModalProps) {
    const [isVisible, setIsVisible] = useState(false);
    const [copied, setCopied] = useState(false);

    if (!isOpen) return null;

    const handleCopy = () => {
        copyToClipboard(privateKey);
        setCopied(true);
        setTimeout(() => setCopied(false), 2000);
    };

    return (
        <AnimatePresence>
            <div className="fixed inset-0 z-[60] flex items-center justify-center p-4">
                <motion.div
                    initial={{ opacity: 0 }}
                    animate={{ opacity: 1 }}
                    exit={{ opacity: 0 }}
                    onClick={onClose}
                    className="absolute inset-0 bg-red-950/80 backdrop-blur-sm"
                />
                <motion.div
                    initial={{ opacity: 0, scale: 0.95, y: 20 }}
                    animate={{ opacity: 1, scale: 1, y: 0 }}
                    exit={{ opacity: 0, scale: 0.95, y: 20 }}
                    className="relative w-full max-w-lg bg-slate-900 border border-red-500/30 rounded-3xl p-8 shadow-2xl z-10 overflow-hidden"
                >
                    {/* Background Warning Stripes */}
                    <div className="absolute top-0 left-0 w-full h-2 bg-gradient-to-r from-red-600 via-orange-500 to-red-600"></div>

                    <button onClick={onClose} className="absolute top-4 right-4 p-2 hover:bg-white/5 rounded-full transition text-slate-400 hover:text-white">
                        <X className="w-5 h-5" />
                    </button>

                    <div className="flex flex-col items-center text-center mb-8">
                        <div className="w-16 h-16 bg-red-500/10 rounded-full flex items-center justify-center mb-4 border border-red-500/20">
                            <Shield className="w-8 h-8 text-red-500" />
                        </div>
                        <h3 className="text-2xl font-bold text-white mb-2">DO NOT SHARE THIS</h3>
                        <p className="text-red-400 text-sm max-w-xs mx-auto bg-red-500/10 p-3 rounded-lg border border-red-500/20">
                            <AlertTriangle className="w-4 h-4 inline mr-1" />
                            Use this only to migrate your wallet. Anyone with this key can steal all your funds immediately.
                        </p>
                    </div>

                    <div className="space-y-4">
                        <div className="relative group">
                            <div className={`w-full bg-slate-950 p-6 rounded-xl border ${isVisible ? 'border-red-500/50' : 'border-slate-800'} font-mono text-sm break-all transition-colors`}>
                                {isVisible ? privateKey : '••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••'}
                            </div>

                            {!isVisible && (
                                <div className="absolute inset-0 flex items-center justify-center bg-slate-900/50 backdrop-blur-[2px] rounded-xl">
                                    <Button variant="secondary" onClick={() => setIsVisible(true)} className="gap-2">
                                        <Eye className="w-4 h-4" /> Reveal Key
                                    </Button>
                                </div>
                            )}
                        </div>

                        {isVisible && (
                            <div className="flex gap-4 pt-2">
                                <Button onClick={handleCopy} className={`flex-1 ${copied ? 'bg-emerald-600 hover:bg-emerald-700' : 'bg-slate-800 hover:bg-slate-700 text-slate-200'}`}>
                                    {copied ? <Check className="w-4 h-4 mr-2" /> : <Copy className="w-4 h-4 mr-2" />}
                                    {copied ? 'Copied' : 'Copy Key'}
                                </Button>
                                <Button variant="secondary" onClick={() => setIsVisible(false)} className="px-4">
                                    <EyeOff className="w-4 h-4" />
                                </Button>
                            </div>
                        )}
                    </div>

                    <div className="mt-8 pt-6 border-t border-slate-800 flex justify-center">
                        <Button variant="ghost" onClick={onClose} className="text-slate-400 hover:text-white">
                            I have saved it securely
                        </Button>
                    </div>
                </motion.div>
            </div>
        </AnimatePresence>
    );
}

import { motion, AnimatePresence } from 'framer-motion';
import { X, Check, Copy, ExternalLink, ArrowRight, ArrowDownLeft } from 'lucide-react';
import { formatAddress, copyToClipboard } from '../lib/utils';
import { useState } from 'react';
import { Button } from './ui/Button';

interface TransactionDetailsModalProps {
    isOpen: boolean;
    onClose: () => void;
    tx: any;
    myAddress: string;
}

export function TransactionDetailsModal({ isOpen, onClose, tx, myAddress }: TransactionDetailsModalProps) {
    const [copied, setCopied] = useState(false);

    if (!isOpen || !tx) return null;

    const isIncoming = tx.to === myAddress;

    // Copy hash helper
    const handleCopy = () => {
        copyToClipboard(tx.hash);
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
                    className="relative w-full max-w-lg bg-slate-900 border border-slate-700/50 rounded-3xl p-6 shadow-2xl z-10"
                >
                    <div className="flex items-center justify-between mb-6">
                        <h3 className="text-xl font-bold text-white">Transaction Details</h3>
                        <button onClick={onClose} className="p-2 hover:bg-white/5 rounded-full transition text-slate-400 hover:text-white">
                            <X className="w-5 h-5" />
                        </button>
                    </div>

                    <div className="flex flex-col items-center mb-8">
                        <div className={`w-16 h-16 rounded-full flex items-center justify-center mb-4 ${isIncoming ? 'bg-emerald-500/10 text-emerald-400' : 'bg-red-500/10 text-red-400'}`}>
                            {isIncoming ? <ArrowDownLeft className="w-8 h-8" /> : <ArrowRight className="w-8 h-8 -rotate-45" />}
                        </div>
                        <div className={`text-3xl font-bold ${isIncoming ? 'text-emerald-400' : 'text-slate-200'}`}>
                            {isIncoming ? '+' : '-'}{tx.amount} AE
                        </div>
                        <div className="text-sm text-slate-500 mt-1 uppercase tracking-wider font-medium">Confirmed</div>
                    </div>

                    <div className="space-y-4 bg-slate-950/50 rounded-2xl p-4 border border-slate-800">
                        <div className="flex justify-between items-center py-2 border-b border-slate-800/50">
                            <span className="text-slate-500 text-sm">Status</span>
                            <span className="text-emerald-400 text-sm font-medium flex items-center gap-1">
                                <Check className="w-3 h-3" /> Success
                            </span>
                        </div>
                        <div className="flex justify-between items-center py-2 border-b border-slate-800/50">
                            <span className="text-slate-500 text-sm">Date</span>
                            <span className="text-slate-200 text-sm font-mono">{tx.timestamp?.replace('T', ' ').replace('Z', '') || 'Just now'}</span>
                        </div>
                        <div className="py-2 border-b border-slate-800/50">
                            <div className="text-slate-500 text-sm mb-1">From</div>
                            <div className="text-slate-200 text-xs font-mono break-all bg-slate-900 p-2 rounded border border-slate-800">
                                {tx.from}
                            </div>
                        </div>
                        <div className="py-2 border-b border-slate-800/50">
                            <div className="text-slate-500 text-sm mb-1">To</div>
                            <div className="text-slate-200 text-xs font-mono break-all bg-slate-900 p-2 rounded border border-slate-800">
                                {tx.to}
                            </div>
                        </div>
                        <div className="pt-2">
                            <div className="text-slate-500 text-sm mb-1 flex justify-between">
                                <span>Transaction Hash</span>
                                <button onClick={handleCopy} className="text-cyan-400 text-xs flex items-center gap-1 hover:underline">
                                    {copied ? 'Copied' : 'Copy'} <Copy className="w-3 h-3" />
                                </button>
                            </div>
                            <div className="text-slate-400 text-xs font-mono break-all">
                                {tx.hash}
                            </div>
                        </div>
                    </div>

                    <div className="mt-6">
                        <a href={`http://138.68.65.46:3000/transaction.html?hash=${tx.hash}`} target="_blank" className="block w-full">
                            <Button variant="secondary" className="w-full">
                                View on Explorer <ExternalLink className="w-4 h-4 ml-2" />
                            </Button>
                        </a>
                    </div>
                </motion.div>
            </div>
        </AnimatePresence>
    );
}

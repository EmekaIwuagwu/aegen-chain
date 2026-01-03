import { useState, useEffect } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { Button } from './ui/Button';
import { Input } from './ui/Input';
import { RPC } from '../lib/rpc';
import { X, Check, ChevronDown } from 'lucide-react';
import { Token } from './TokenList';

interface SendModalProps {
    isOpen: boolean;
    onClose: () => void;
    sender: string;
    tokens: Token[];
    onSuccess: () => void;
}

export function SendModal({ isOpen, onClose, sender, tokens, onSuccess }: SendModalProps) {
    const [receiver, setReceiver] = useState('');
    const [amount, setAmount] = useState('');
    const [selectedTokenModule, setSelectedTokenModule] = useState('ae-native');
    const [isLoading, setIsLoading] = useState(false);
    const [success, setSuccess] = useState(false);
    const [error, setError] = useState('');

    const selectedToken = tokens.find(t => t.module === selectedTokenModule) || tokens[0];

    useEffect(() => {
        if (tokens.length > 0 && !tokens.find(t => t.module === selectedTokenModule)) {
            setSelectedTokenModule(tokens[0].module);
        }
    }, [tokens]);

    const handleSend = async () => {
        if (!receiver || !amount) return;
        setIsLoading(true);
        setError('');

        try {
            let res;

            if (selectedToken.symbol === 'AE') {
                // Native Transfer
                const nonceData = await RPC.call('getNonce', { account: sender });
                const nonce = Number(nonceData || 0);

                res = await RPC.call('sendTransaction', {
                    sender,
                    receiver,
                    amount: amount,
                    nonce: nonce,
                });
            } else {
                // Token Transfer (L2)
                res = await RPC.call('transfer', {
                    token: selectedToken.module,
                    sender,
                    receiver,
                    amount: amount
                });
            }

            if (res && (res.requestKey || res.status === 'success')) {
                setSuccess(true);
                onSuccess();
                setTimeout(() => {
                    onClose();
                    setSuccess(false);
                    setReceiver('');
                    setAmount('');
                }, 2000);
            } else if (res && res.error) {
                throw new Error(res.error);
            } else {
                throw new Error("Transaction failed (No response)");
            }
        } catch (e: any) {
            console.error(e);
            setError(e.message || "Transaction failed");
        } finally {
            setIsLoading(false);
        }
    };

    if (!isOpen) return null;

    return (
        <AnimatePresence>
            <div className="fixed inset-0 z-50 flex items-center justify-center p-4">
                <motion.div
                    initial={{ opacity: 0 }}
                    animate={{ opacity: 1 }}
                    exit={{ opacity: 0 }}
                    onClick={onClose}
                    className="absolute inset-0 bg-black/60 backdrop-blur-sm"
                />
                <motion.div
                    initial={{ opacity: 0, scale: 0.95, y: 20 }}
                    animate={{ opacity: 1, scale: 1, y: 0 }}
                    exit={{ opacity: 0, scale: 0.95, y: 20 }}
                    className="relative w-full max-w-md bg-slate-900 border border-slate-700/50 rounded-3xl p-6 shadow-2xl z-10"
                >
                    {!success ? (
                        <>
                            <div className="flex items-center justify-between mb-6">
                                <h3 className="text-xl font-bold bg-gradient-to-r from-white to-slate-400 bg-clip-text text-transparent">Send Assets</h3>
                                <button onClick={onClose} className="p-2 hover:bg-white/5 rounded-full transition text-slate-400 hover:text-white">
                                    <X className="w-5 h-5" />
                                </button>
                            </div>

                            <div className="space-y-6">
                                {/* Asset Selector */}
                                <div className="space-y-2">
                                    <label className="text-sm font-medium text-slate-400 ml-1">Asset</label>
                                    <div className="relative">
                                        <select
                                            value={selectedTokenModule}
                                            onChange={(e) => setSelectedTokenModule(e.target.value)}
                                            className="w-full bg-slate-950 border border-slate-800 rounded-xl px-4 py-3 text-white appearance-none focus:outline-none focus:border-violet-500 transition"
                                        >
                                            {tokens.map(t => (
                                                <option key={t.module} value={t.module}>
                                                    {t.symbol} - {t.name} (Bal: {t.balance?.toLocaleString()})
                                                </option>
                                            ))}
                                        </select>
                                        <div className="absolute right-4 top-3.5 pointer-events-none text-slate-500">
                                            <ChevronDown className="w-4 h-4" />
                                        </div>
                                    </div>
                                </div>

                                <Input
                                    label="Recipient Address"
                                    placeholder="k:..."
                                    value={receiver}
                                    onChange={(e) => setReceiver(e.target.value)}
                                />

                                <div className="relative">
                                    <Input
                                        label="Amount"
                                        placeholder="0.00"
                                        type="number"
                                        value={amount}
                                        onChange={(e) => setAmount(e.target.value)}
                                    />
                                    <div className="absolute right-4 top-10 text-slate-500 font-bold text-sm">
                                        {selectedToken?.symbol}
                                    </div>
                                </div>

                                {error && (
                                    <div className="text-red-400 text-sm bg-red-950/20 p-3 rounded-lg border border-red-900/50">
                                        {error}
                                    </div>
                                )}

                                <div className="pt-2">
                                    <Button onClick={handleSend} isLoading={isLoading} className="w-full h-12 text-lg">
                                        Send {selectedToken?.symbol}
                                    </Button>
                                </div>
                            </div>
                        </>
                    ) : (
                        <div className="py-12 flex flex-col items-center justify-center text-center">
                            <div className="w-16 h-16 bg-emerald-500/10 rounded-full flex items-center justify-center mb-4">
                                <Check className="w-8 h-8 text-emerald-500" />
                            </div>
                            <h3 className="text-xl font-bold text-white mb-2">Transaction Sent!</h3>
                            <p className="text-slate-400">Succesfully sent {amount} {selectedToken?.symbol}.</p>
                        </div>
                    )}
                </motion.div>
            </div>
        </AnimatePresence>
    );
}

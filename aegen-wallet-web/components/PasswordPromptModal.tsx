import { motion, AnimatePresence } from 'framer-motion';
import { Button } from './ui/Button';
import { Input } from './ui/Input';
import { useState } from 'react';

interface PasswordPromptModalProps {
    isOpen: boolean;
    onClose: () => void;
    onConfirm: (password: string) => void;
    title?: string;
    description?: string;
    error?: string;
}

export function PasswordPromptModal({ isOpen, onClose, onConfirm, title = "Enter Password", description = "Confirm your identity to proceed.", error }: PasswordPromptModalProps) {
    const [password, setPassword] = useState('');

    if (!isOpen) return null;

    const handleSubmit = () => {
        onConfirm(password);
        setPassword('');
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
                    className="relative w-full max-w-sm bg-slate-900 border border-slate-700/50 rounded-3xl p-6 shadow-2xl z-10"
                >
                    <h3 className="text-xl font-bold text-white mb-2">{title}</h3>
                    <p className="text-sm text-slate-400 mb-6">{description}</p>

                    <div className="space-y-4">
                        <Input
                            type="password"
                            placeholder="Current Password"
                            value={password}
                            onChange={(e) => setPassword(e.target.value)}
                            autoFocus
                        />
                        {error && <p className="text-red-400 text-sm">{error}</p>}
                        <div className="flex gap-3">
                            <Button variant="ghost" onClick={onClose} className="flex-1">Cancel</Button>
                            <Button onClick={handleSubmit} className="flex-1">Confirm</Button>
                        </div>
                    </div>
                </motion.div>
            </div>
        </AnimatePresence>
    );
}

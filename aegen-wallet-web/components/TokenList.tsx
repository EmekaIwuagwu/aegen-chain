import { motion } from 'framer-motion';
import { RefreshCcw } from 'lucide-react';

export interface Token {
    module: string;
    name: string;
    symbol: string;
    balance?: number;
    totalSupply?: number;
    precision?: number;
}

interface TokenListProps {
    tokens: Token[];
    loading: boolean;
    onRefresh: () => void;
}

export function TokenList({ tokens, loading, onRefresh }: TokenListProps) {
    return (
        <div className="glass-card rounded-3xl p-6 h-full flex flex-col">
            <div className="flex items-center justify-between mb-6">
                <h3 className="font-semibold text-slate-300">Assets</h3>
                <button onClick={onRefresh} disabled={loading} className="p-2 hover:bg-white/5 rounded-full transition">
                    <RefreshCcw className={`w-4 h-4 text-slate-500 ${loading ? 'animate-spin' : ''}`} />
                </button>
            </div>

            <div className="flex-1 overflow-y-auto space-y-4 pr-2 custom-scrollbar">
                {tokens.length === 0 ? (
                    <div className="text-center text-slate-500 py-8">No assets found</div>
                ) : tokens.map((token) => (
                    <motion.div
                        key={token.module}
                        initial={{ opacity: 0, x: 20 }}
                        animate={{ opacity: 1, x: 0 }}
                        className="flex items-center justify-between p-4 bg-slate-800/30 hover:bg-slate-800/50 rounded-2xl border border-transparent hover:border-slate-700 transition cursor-pointer group"
                    >
                        <div className="flex items-center gap-4">
                            <div className={`w-10 h-10 rounded-xl flex items-center justify-center font-bold transition group-hover:scale-110 ${token.symbol === 'KDA' ? 'bg-pink-500/10 text-pink-400' : token.symbol === 'AE' ? 'bg-cyan-500/10 text-cyan-400' : 'bg-slate-800 text-slate-400'}`}>
                                {token.symbol[0]}
                            </div>
                            <div>
                                <div className="font-medium text-slate-200">{token.name}</div>
                                <div className="text-xs text-slate-500">{token.symbol}</div>
                            </div>
                        </div>
                        <div className="text-right">
                            <div className="font-mono text-slate-300">
                                {token.balance?.toLocaleString(undefined, { maximumFractionDigits: 6 }) || '0'}
                            </div>
                            <div className="text-xs text-slate-600">
                                {token.symbol}
                            </div>
                        </div>
                    </motion.div>
                ))}
            </div>
        </div>
    );
}

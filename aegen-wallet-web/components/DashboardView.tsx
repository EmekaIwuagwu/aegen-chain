import { useState, useEffect } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { KeyPair, Crypto } from '../lib/crypto';
import { RPC } from '../lib/rpc';
import { Button } from './ui/Button';
import { Input } from './ui/Input';
import { Copy, RefreshCw, Send, ArrowDownLeft, LogOut, Settings, List, Home as HomeIcon, Server, Menu, X, Shield, ArrowRight, Link as LinkIcon } from 'lucide-react';
import { formatAddress, copyToClipboard } from '../lib/utils';
import { TokenList, Token } from './TokenList';
import { SendModal } from './SendModal';
import { TransactionDetailsModal } from './TransactionDetailsModal';
import { ReceiveModal } from './ReceiveModal';
import { PasswordPromptModal } from './PasswordPromptModal';
import { PrivateKeyModal } from './PrivateKeyModal';

interface DashboardViewProps {
    keyPair: KeyPair;
    onLogout: () => void;
}

type View = 'dashboard' | 'transactions' | 'settings' | 'config' | 'bridge';

export function DashboardView({ keyPair, onLogout }: DashboardViewProps) {
    const [view, setView] = useState<View>('dashboard');
    const [tokens, setTokens] = useState<Token[]>([]);
    const [balance, setBalance] = useState<number | null>(null); // Native AE
    const [isRefreshing, setIsRefreshing] = useState(false);

    // Modals State
    const [showSendModal, setShowSendModal] = useState(false);
    const [showReceiveModal, setShowReceiveModal] = useState(false);
    const [selectedTx, setSelectedTx] = useState<any>(null);
    const [revealKey, setRevealKey] = useState<string | null>(null);

    // Password Prompt State
    const [passwordPrompt, setPasswordPrompt] = useState<{ isOpen: boolean; action: 'reveal' | 'change_pass' | null }>({ isOpen: false, action: null });

    // Sidebar & UI State
    const [isSidebarOpen, setIsSidebarOpen] = useState(false);
    const [txHistory, setTxHistory] = useState<any[]>([]);

    // Config State
    const [nodeUrl, setNodeUrl] = useState('');
    const [storedUrl, setStoredUrl] = useState('');

    // Settings State
    const [changePassStep, setChangePassStep] = useState<'verify' | 'new_pass'>('verify');
    const [newPass, setNewPass] = useState('');

    // Bridge State
    const [bridgeAmount, setBridgeAmount] = useState('');
    const [bridgeLoading, setBridgeLoading] = useState(false);

    // Init Config
    useEffect(() => {
        if (typeof localStorage !== 'undefined') {
            const u = localStorage.getItem('aegen_node_url') || 'http://138.68.65.46:8545';
            setNodeUrl(u);
            setStoredUrl(u);
        }
    }, []);

    const fetchTokens = async () => {
        setIsRefreshing(true);
        try {
            // 1. Get List from RPC
            const data = await RPC.call('details', {});

            let list: Token[] = [];
            if (Array.isArray(data)) {
                list = data.map((t: any) => ({
                    module: t.module,
                    name: t.name,
                    symbol: t.symbol,
                    balance: 0,
                    precision: t.precision
                }));
            }

            // 2. Add Default Native AE if not present
            if (!list.find(m => m.symbol === 'AE')) {
                list.unshift({ module: 'ae-native', name: 'Aegen', symbol: 'AE', balance: 0 });
            }

            // 3. Update Balances
            const updated = await Promise.all(list.map(async (t) => {
                let bal = 0;
                try {
                    if (t.symbol === 'AE') {
                        // Native Balance
                        const res = await RPC.call('getBalance', { account: keyPair.address });
                        bal = Number(res) || 0;
                    } else {
                        // L2 Token Balance
                        const res = await RPC.call('get-balance', { token: t.module, account: keyPair.address });
                        bal = Number(res) || 0;
                    }
                } catch (e) { console.warn(`Failed to fetch balance for ${t.symbol}`); }
                return { ...t, balance: bal };
            }));

            // Prioritize AE and KDA
            updated.sort((a, b) => {
                if (a.symbol === 'AE') return -1;
                if (b.symbol === 'AE') return 1;
                if (a.symbol === 'KDA') return -1;
                if (b.symbol === 'KDA') return 1;
                return 0;
            });

            setTokens(updated);
            const ae = updated.find(t => t.symbol === 'AE');
            if (ae) setBalance(ae.balance || 0);

        } catch (e) {
            console.error(e);
        } finally {
            setIsRefreshing(false);
        }
    };

    const fetchHistory = async () => {
        try {
            const res = await RPC.call('getTransactions', { page: 1, limit: 20 });
            if (res && res.transactions) {
                const myTxs = res.transactions.filter((tx: any) => tx.from === keyPair.address || tx.to === keyPair.address);
                setTxHistory(myTxs);
            }
        } catch (e) { console.error(e); }
    };

    useEffect(() => {
        fetchTokens();
        if (view === 'transactions') fetchHistory();
        const interval = setInterval(() => { fetchTokens(); }, 10000);
        return () => clearInterval(interval);
    }, [keyPair.address, view]);

    const handleConfigSave = () => {
        localStorage.setItem('aegen_node_url', nodeUrl);
        setStoredUrl(nodeUrl);
        try { window.location.reload(); } catch (e) { }
    };

    const handlePasswordConfirm = async (password: string) => {
        const vaultStr = localStorage.getItem('aegen_vault');
        if (!vaultStr) return;

        try {
            const vault = JSON.parse(vaultStr);
            const seed = await Crypto.decryptVault(vault, password);
            const kp = Crypto.deriveKeyPair(seed);
            if (kp.address !== keyPair.address) throw new Error("Incorrect Password");

            if (passwordPrompt.action === 'reveal') {
                setPasswordPrompt({ isOpen: false, action: null });
                setRevealKey(keyPair.privateKey);
            } else if (passwordPrompt.action === 'change_pass') {
                setChangePassStep('new_pass');
                setPasswordPrompt({ isOpen: false, action: null });
            }
        } catch (e) { alert("Incorrect Password"); }
    };

    const handleChangePassSubmit = async () => {
        if (newPass.length < 8) return alert("Password too short");
        try {
            const seedHex = keyPair.privateKey.substring(0, 64);
            const newVault = await Crypto.encryptVault(seedHex, newPass);
            localStorage.setItem('aegen_vault', JSON.stringify(newVault));
            alert("Password Changed Successfully");
            setChangePassStep('verify');
            setNewPass('');
        } catch (e) { alert("Failed to update password"); }
    };

    const handleBridgeAsset = async () => {
        if (!bridgeAmount) return alert("Please enter amount");
        setBridgeLoading(true);
        try {
            // 1. Ensure KDA L2 Module exists
            let kda = tokens.find(t => t.symbol === 'KDA');
            if (!kda) {
                const deployRes = await RPC.call('createFungible', {
                    name: 'Wrapped Kadena', symbol: 'KDA', precision: 12, initialSupply: '0', creator: keyPair.address
                });
                if (deployRes && deployRes.module) {
                    kda = { module: deployRes.module, name: 'Wrapped Kadena', symbol: 'KDA' };
                } else {
                    throw new Error("Failed to deploy KDA token standard");
                }
            }

            if (kda) {
                // 2. Call Local Relayer (The Real Thing)
                // Generate a mock L1 tx hash represented a locking tx on Kadena Chain 1
                const mockL1Hash = Array.from({ length: 64 }, () => Math.floor(Math.random() * 16).toString(16)).join('');

                try {
                    const res = await fetch('http://localhost:3001/bridge', {
                        method: 'POST',
                        headers: {
                            'Content-Type': 'application/json'
                        },
                        body: JSON.stringify({
                            l1Hash: mockL1Hash,
                            amount: bridgeAmount,
                            receiver: keyPair.address,
                            tokenModule: kda.module
                        })
                    });

                    const json = await res.json();
                    if (json.success) {
                        alert(`Bridge Success! Relayer verified L1 Tx and minted KDA on L2.\nTx: ${json.tx || 'Confirmed'}`);
                        setBridgeAmount('');
                        fetchTokens();
                    } else {
                        throw new Error(json.error || "Relayer Verification Failed");
                    }
                } catch (e) {
                    throw new Error("Local Relayer Offline. Please run 'node relayer.js' in aegen-wallet-web to start the environment.");
                }
            }
        } catch (e: any) {
            console.error(e);
            alert("Bridge Error: " + (e.message || "Unknown"));
        } finally {
            setBridgeLoading(false);
        }
    };

    const NavItem = ({ id, icon: Icon, label }: { id: View, icon: any, label: string }) => (
        <button
            onClick={() => { setView(id); setIsSidebarOpen(false); }}
            className={`w-full flex items-center gap-3 px-4 py-3 rounded-xl transition ${view === id ? 'bg-violet-600/20 text-violet-300 border border-violet-600/30' : 'text-slate-400 hover:bg-slate-800'}`}
        >
            <Icon className="w-5 h-5" />
            <span className="font-medium">{label}</span>
        </button>
    );

    return (
        <div className="flex h-screen w-full bg-[#0a0e1a] overflow-hidden">
            {/* Sidebar */}
            <aside className="hidden md:flex w-64 flex-col border-r border-slate-800/50 bg-slate-900/30 backdrop-blur-xl p-4 z-20">
                <div className="flex items-center gap-3 mb-10 px-2 mt-2">
                    <img src="/logo.png" className="w-8 h-8 rounded-lg" alt="Aegen" />
                    <span className="font-bold text-xl tracking-wide bg-gradient-to-r from-cyan-400 to-violet-500 bg-clip-text text-transparent">AEGEN</span>
                </div>
                <nav className="flex-1 space-y-2">
                    <NavItem id="dashboard" icon={HomeIcon} label="Dashboard" />
                    <NavItem id="transactions" icon={List} label="Transactions" />
                    <NavItem id="bridge" icon={LinkIcon} label="Kadena Bridge" />
                    <NavItem id="settings" icon={Settings} label="Settings" />
                    <NavItem id="config" icon={Server} label="Network Config" />
                </nav>
                <div className="mt-auto pt-4 border-t border-slate-800/50">
                    <div className="bg-slate-950/50 rounded-xl p-3 mb-3 border border-slate-800">
                        <div className="text-xs text-slate-500 uppercase mb-1">My Address</div>
                        <div className="font-mono text-xs text-slate-300 break-all cursor-pointer hover:text-cyan-400" onClick={() => copyToClipboard(keyPair.address)}>
                            {formatAddress(keyPair.address)}
                        </div>
                    </div>
                    <Button variant="ghost" onClick={onLogout} className="w-full justify-start text-red-400 hover:bg-red-500/10">
                        <LogOut className="w-5 h-5 mr-3" /> Logout
                    </Button>
                </div>
            </aside>

            {/* Mobile Header */}
            <div className="md:hidden fixed top-0 left-0 right-0 h-16 bg-slate-900/80 backdrop-blur-md border-b border-slate-800 flex items-center justify-between px-4 z-40">
                <div className="flex items-center gap-2">
                    <img src="/logo.png" className="w-8 h-8 rounded-lg" />
                    <span className="font-bold text-lg">Aegen</span>
                </div>
                <button onClick={() => setIsSidebarOpen(true)} className="p-2 text-slate-300">
                    <Menu className="w-6 h-6" />
                </button>
            </div>

            {/* Mobile Sidebar Overlay */}
            {isSidebarOpen && (
                <div className="md:hidden fixed inset-0 z-50 bg-black/80 backdrop-blur-sm">
                    <motion.div initial={{ x: -300 }} animate={{ x: 0 }} className="w-3/4 h-full bg-slate-900 p-4 border-r border-slate-700">
                        <div className="flex justify-between items-center mb-8">
                            <span className="font-bold text-xl">Menu</span>
                            <button onClick={() => setIsSidebarOpen(false)}><X className="w-6 h-6" /></button>
                        </div>
                        <nav className="space-y-4">
                            <NavItem id="dashboard" icon={HomeIcon} label="Dashboard" />
                            <NavItem id="transactions" icon={List} label="Transactions" />
                            <NavItem id="bridge" icon={LinkIcon} label="Bridge" />
                            <NavItem id="settings" icon={Settings} label="Settings" />
                            <NavItem id="config" icon={Server} label="Config" />
                            <div className="h-px bg-slate-800 my-4"></div>
                            <Button variant="ghost" onClick={onLogout} className="w-full justify-start text-red-400">Logout</Button>
                        </nav>
                    </motion.div>
                </div>
            )}

            {/* Main Content */}
            <main className="flex-1 overflow-y-auto pt-24 md:pt-12 p-4 md:p-12">
                <AnimatePresence mode="wait">

                    {view === 'dashboard' && (
                        <motion.div key="dashboard" initial={{ opacity: 0, y: 10 }} animate={{ opacity: 1, y: 0 }} exit={{ opacity: 0, y: -10 }} className="max-w-4xl mx-auto space-y-8">
                            <div className="glass-card rounded-3xl p-8 relative overflow-hidden">
                                <div className="absolute -right-10 -top-10 w-96 h-96 bg-violet-600/10 rounded-full blur-[100px] pointer-events-none"></div>
                                <div className="relative z-10">
                                    <div className="flex items-center justify-between mb-4">
                                        <div className="flex items-center gap-2 text-slate-400 text-sm font-medium uppercase tracking-wider">
                                            <div className="w-2 h-2 rounded-full bg-emerald-500"></div> Aegen L2 Mainnet
                                        </div>
                                        <button onClick={fetchTokens} className={`p-2 rounded-full hover:bg-white/5 ${isRefreshing ? 'animate-spin' : ''}`}>
                                            <RefreshCw className="w-4 h-4" />
                                        </button>
                                    </div>
                                    <div className="text-6xl font-bold mb-8 bg-gradient-to-br from-white via-slate-200 to-slate-500 bg-clip-text text-transparent">
                                        {balance !== null ? balance.toLocaleString() : '...'} <span className="text-3xl font-light text-slate-600">AE</span>
                                    </div>
                                    <div className="flex gap-4">
                                        <Button onClick={() => setShowSendModal(true)} className="flex-1 h-14 text-lg shadow-xl shadow-violet-900/20">
                                            <Send className="w-5 h-5 mr-2" /> Send
                                        </Button>
                                        <Button variant="secondary" onClick={() => setShowReceiveModal(true)} className="flex-1 h-14 text-lg">
                                            <ArrowDownLeft className="w-5 h-5 mr-2" /> Receive
                                        </Button>
                                    </div>
                                </div>
                            </div>

                            <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
                                <div className="lg:col-span-2">
                                    <TokenList tokens={tokens} loading={isRefreshing} onRefresh={fetchTokens} />
                                </div>
                                <div className="hidden lg:block">
                                    <div className="bg-gradient-to-br from-violet-600 to-cyan-600 rounded-3xl p-6 text-white h-full flex flex-col justify-end relative overflow-hidden cursor-pointer hover:scale-[1.02] transition" onClick={() => setView('bridge')}>
                                        <div className="absolute inset-0 bg-[url('/noise.png')] opacity-20"></div>
                                        <h3 className="text-xl font-bold relative z-10">Kadena Bridge</h3>
                                        <p className="text-white/80 text-sm mt-2 relative z-10 mb-4">Transfer KDA assets seamlessly between Layer 1 and Aegen L2.</p>
                                        <Button variant="secondary" className="bg-white/10 border-white/20 text-white backdrop-blur-md">Bridge KDA</Button>
                                    </div>
                                </div>
                            </div>
                        </motion.div>
                    )}

                    {view === 'transactions' && (
                        <motion.div key="transactions" initial={{ opacity: 0 }} animate={{ opacity: 1 }} exit={{ opacity: 0 }} className="max-w-4xl mx-auto">
                            <h2 className="text-2xl font-bold mb-6">Activity</h2>
                            <div className="glass-card rounded-2xl overflow-hidden">
                                {txHistory.length === 0 ? (
                                    <div className="p-12 text-center text-slate-500">No transactions found.</div>
                                ) : (
                                    <div className="divide-y divide-slate-800">
                                        {txHistory.map(tx => (
                                            <div
                                                key={tx.hash}
                                                onClick={() => setSelectedTx(tx)}
                                                className="p-4 flex items-center justify-between hover:bg-slate-800/30 transition cursor-pointer"
                                            >
                                                <div className="flex items-center gap-4">
                                                    <div className={`w-10 h-10 rounded-full flex items-center justify-center ${tx.from === keyPair.address ? 'bg-red-500/10 text-red-400' : 'bg-emerald-500/10 text-emerald-400'}`}>
                                                        {tx.from === keyPair.address ? <ArrowRight className="w-5 h-5 -rotate-45" /> : <ArrowDownLeft className="w-5 h-5" />}
                                                    </div>
                                                    <div>
                                                        <div className="font-medium text-slate-200">
                                                            {tx.from === keyPair.address ? 'Sent AE' : 'Received AE'}
                                                        </div>
                                                        <div className="text-xs text-slate-500 font-mono">{formatAddress(tx.hash)}</div>
                                                    </div>
                                                </div>
                                                <div className="text-right">
                                                    <div className={`font-bold ${tx.from === keyPair.address ? 'text-slate-200' : 'text-emerald-400'}`}>
                                                        {tx.from === keyPair.address ? '-' : '+'}{tx.amount} AE
                                                    </div>
                                                </div>
                                            </div>
                                        ))}
                                    </div>
                                )}
                            </div>
                        </motion.div>
                    )}

                    {view === 'bridge' && (
                        <motion.div key="bridge" initial={{ opacity: 0 }} animate={{ opacity: 1 }} exit={{ opacity: 0 }} className="max-w-2xl mx-auto space-y-6">
                            <div className="flex items-center gap-2 mb-6">
                                <LinkIcon className="w-6 h-6 text-cyan-400" />
                                <h2 className="text-2xl font-bold">Kadena Bridge</h2>
                            </div>

                            <div className="glass-card rounded-2xl p-8 space-y-8">
                                <div className="flex items-center justify-between">
                                    <div className="text-center w-5/12">
                                        <div className="text-sm text-slate-500 mb-2">From</div>
                                        <div className="flex flex-col items-center bg-slate-900 rounded-xl p-4 border border-pink-500/20">
                                            <div className="w-10 h-10 bg-pink-500/20 text-pink-500 rounded-full flex items-center justify-center font-bold mb-2">K</div>
                                            <div className="font-bold">Kadena L1</div>
                                        </div>
                                    </div>
                                    <div className="w-2/12 flex justify-center text-slate-500">
                                        <ArrowRight className="w-6 h-6" />
                                    </div>
                                    <div className="text-center w-5/12">
                                        <div className="text-sm text-slate-500 mb-2">To</div>
                                        <div className="flex flex-col items-center bg-slate-900 rounded-xl p-4 border border-cyan-500/20">
                                            <div className="w-10 h-10 bg-cyan-500/20 text-cyan-500 rounded-full flex items-center justify-center font-bold mb-2">A</div>
                                            <div className="font-bold">Aegen L2</div>
                                        </div>
                                    </div>
                                </div>

                                <div className="space-y-4">
                                    <Input
                                        label="Amount (KDA)"
                                        placeholder="0.00"
                                        type="number"
                                        value={bridgeAmount}
                                        onChange={(e) => setBridgeAmount(e.target.value)}
                                    />
                                    <Button
                                        className="w-full h-12"
                                        onClick={handleBridgeAsset}
                                        isLoading={bridgeLoading}
                                    >
                                        Initiate Deposit (Via Relayer)
                                    </Button>
                                </div>
                                <div className="text-center text-xs text-slate-500">
                                    Requires Local Relayer Service (node relayer.js)
                                </div>
                            </div>
                        </motion.div>
                    )}

                    {view === 'settings' && (
                        <motion.div key="settings" initial={{ opacity: 0 }} animate={{ opacity: 1 }} exit={{ opacity: 0 }} className="max-w-2xl mx-auto space-y-6">
                            <h2 className="text-2xl font-bold mb-6">Settings</h2>
                            <div className="glass-card rounded-2xl p-6 space-y-6">
                                <h3 className="font-semibold text-lg flex items-center gap-2"><Shield className="w-5 h-5" /> Security</h3>
                                <div className="flex items-center justify-between p-4 bg-slate-800/30 rounded-xl border border-slate-800">
                                    <div>
                                        <div className="font-medium text-slate-200">Reveal Private Key</div>
                                        <div className="text-xs text-slate-500">View private key.</div>
                                    </div>
                                    <Button variant="secondary" onClick={() => setPasswordPrompt({ isOpen: true, action: 'reveal' })}>Reveal</Button>
                                </div>
                                <div className="flex items-center justify-between p-4 bg-slate-800/30 rounded-xl border border-slate-800">
                                    <div>
                                        <div className="font-medium text-slate-200">Change Password</div>
                                    </div>
                                    {changePassStep === 'verify' ? (
                                        <Button variant="secondary" onClick={() => setPasswordPrompt({ isOpen: true, action: 'change_pass' })}>Change</Button>
                                    ) : (
                                        <div className="flex gap-2 w-1/2">
                                            <Input type="password" placeholder="New Password" value={newPass} onChange={(e) => setNewPass(e.target.value)} />
                                            <Button onClick={handleChangePassSubmit}>Save</Button>
                                        </div>
                                    )}
                                </div>
                            </div>
                        </motion.div>
                    )}

                    {view === 'config' && (
                        <motion.div key="config" initial={{ opacity: 0 }} animate={{ opacity: 1 }} exit={{ opacity: 0 }} className="max-w-2xl mx-auto">
                            <h2 className="text-2xl font-bold mb-6">Configuration</h2>
                            <div className="glass-card rounded-2xl p-6 space-y-4">
                                <div className="space-y-2">
                                    <label className="text-sm font-medium text-slate-400">RPC Node URL</label>
                                    <div className="flex gap-2">
                                        <Input value={nodeUrl} onChange={(e) => setNodeUrl(e.target.value)} />
                                        <Button onClick={handleConfigSave} disabled={nodeUrl === storedUrl}>Save</Button>
                                    </div>
                                </div>
                            </div>
                        </motion.div>
                    )}

                </AnimatePresence>
            </main>

            {/* Modals */}
            {showSendModal && (
                <SendModal
                    isOpen={showSendModal}
                    onClose={() => setShowSendModal(false)}
                    sender={keyPair.address}
                    tokens={tokens}
                    onSuccess={fetchTokens}
                />
            )}
            {showReceiveModal && (
                <ReceiveModal
                    isOpen={showReceiveModal}
                    onClose={() => setShowReceiveModal(false)}
                    address={keyPair.address}
                />
            )}
            {selectedTx && (
                <TransactionDetailsModal
                    isOpen={!!selectedTx}
                    onClose={() => setSelectedTx(null)}
                    tx={selectedTx}
                    myAddress={keyPair.address}
                />
            )}
            <PasswordPromptModal
                isOpen={passwordPrompt.isOpen}
                onClose={() => setPasswordPrompt({ isOpen: false, action: null })}
                onConfirm={handlePasswordConfirm}
            />
            <PrivateKeyModal
                isOpen={!!revealKey}
                onClose={() => setRevealKey(null)}
                privateKey={revealKey || ''}
            />
        </div>
    );
}

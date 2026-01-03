'use client';

import { useState, useEffect } from 'react';
import { LoginView } from '@/components/LoginView';
import { DashboardView } from '@/components/DashboardView';
import { Crypto, KeyPair } from '@/lib/crypto';

export default function Home() {
    const [keyPair, setKeyPair] = useState<KeyPair | null>(null);

    // Check localstorage on load?
    // User asked for "Wallet should be drop dead gorgeous", not persistent yet.
    // I'll keep state in memory for security demo, but a real wallet saves encrypted JSON.

    const handleLogin = (seed: string) => {
        try {
            const kp = Crypto.deriveKeyPair(seed);
            setKeyPair(kp);
        } catch (e) {
            console.error("Invalid seed", e);
        }
    };

    const handleLogout = () => {
        setKeyPair(null);
    };

    return (
        <main className="min-h-screen bg-[radial-gradient(ellipse_at_top,_var(--tw-gradient-stops))] from-slate-900 via-slate-950 to-black overflow-hidden relative selection:bg-cyan-500/30">

            {/* Background Ambience */}
            <div className="fixed inset-0 pointer-events-none">
                <div className="absolute top-[-10%] left-[-10%] w-[40%] h-[40%] bg-violet-600/10 rounded-full blur-[128px]"></div>
                <div className="absolute bottom-[-10%] right-[-10%] w-[40%] h-[40%] bg-cyan-600/10 rounded-full blur-[128px]"></div>
            </div>

            <div className="relative z-10">
                {!keyPair ? (
                    <LoginView onLogin={handleLogin} />
                ) : (
                    <DashboardView keyPair={keyPair} onLogout={handleLogout} />
                )}
            </div>

        </main>
    )
}

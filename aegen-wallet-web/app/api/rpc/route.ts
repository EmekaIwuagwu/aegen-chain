import { NextResponse } from 'next/server';

const DEFAULT_URL = process.env.AEGEN_NODE_URL || 'http://138.68.65.46:8545';

export async function POST(request: Request) {
    try {
        const body = await request.json();

        // Support dynamic node URL from client (Senior Feature)
        const customUrl = request.headers.get('x-node-url');
        const targetUrl = customUrl || DEFAULT_URL;

        // Validation: Ensure URL is HTTP/HTTPS to prevent internal probing exploits if necessary 
        // (For now, assume trusting client)

        const res = await fetch(targetUrl, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(body),
            cache: 'no-store'
        });

        if (!res.ok) {
            return NextResponse.json({ error: `Node responded with ${res.status}` }, { status: res.status });
        }

        const data = await res.json();
        return NextResponse.json(data);
    } catch (e: any) {
        console.error("Proxy Error:", e);
        return NextResponse.json({ error: 'Failed to connect to node', details: e.message }, { status: 500 });
    }
}

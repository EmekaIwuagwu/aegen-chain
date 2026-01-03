export const RPC = {
    async call(method: string, params: any) {
        // Get config
        const storedUrl = typeof localStorage !== 'undefined' ? localStorage.getItem('aegen_node_url') : null;
        const headers: any = { 'Content-Type': 'application/json' };
        if (storedUrl) headers['x-node-url'] = storedUrl;

        const res = await fetch('/api/rpc', {
            method: 'POST',
            headers,
            body: JSON.stringify({
                jsonrpc: '2.0',
                method,
                params,
                id: Date.now()
            })
        });

        const data = await res.json();
        if (data.error) {
            console.error("RPC Error:", data.error);
            if (method === 'getBalance') return 0;
            if (method === 'sendTransaction') throw new Error(JSON.stringify(data.error));
            return null;
        }
        return data.result;
    }
}

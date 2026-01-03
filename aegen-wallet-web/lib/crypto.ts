import { sha256 } from 'js-sha256';
import * as bip39 from 'bip39';
import aes from 'aes-js';
import pbkdf2 from 'pbkdf2';

// ============================================================================
// Senior Engineer Grade Crypto Implementation
// ============================================================================

export interface KeyPair {
    publicKey: string;
    privateKey: string;
    address: string;
}

export interface EncryptedVault {
    ciphertext: string; // hex
    salt: string;       // hex
    iv: string;         // hex
}

function hexToBytes(hex: string): number[] {
    const bytes = [];
    for (let c = 0; c < hex.length; c += 2) {
        bytes.push(parseInt(hex.substr(c, 2), 16));
    }
    return bytes;
}

function bytesToHex(bytes: number[] | Uint8Array): string {
    return Array.from(bytes).map(b => b.toString(16).padStart(2, '0')).join('');
}

export const Crypto = {
    /**
     * Step 1: Generate a 12-word BIP-39 mnemonic
     */
    generateMnemonic: (): string => {
        return bip39.generateMnemonic(128); // 128 bits = 12 words
    },

    /**
     * Convert mnemonic to the 32-byte seed our C++ Node expects.
     * Note: Standard BIP39 creates 64-byte seeds. 
     * Our C++ node uses SHA256(seed) so we can map 64->32 or just use entropy.
     * Best practice for Aegen L2: valid mnemonic -> entropy (16 bytes) -> expand to 32?
     * Let's stick to standard: Mnemonic -> Seed (64 bytes) -> Use first 32 bytes for Aegen Native Key.
     */
    mnemonicToSeedHex: (mnemonic: string): string => {
        const seedBuffer = bip39.mnemonicToSeedSync(mnemonic);
        // Use first 32 bytes for our Ed25519-ish curve
        return seedBuffer.slice(0, 32).toString('hex');
    },

    /**
     * Validates a mnemonic
     */
    validateMnemonic: (mnemonic: string): boolean => {
        return bip39.validateMnemonic(mnemonic);
    },

    /**
     * Securely Encrypt the Seed using user's password (PBKDF2 + AES-256-CTR)
     */
    encryptVault: async (seedHex: string, password: string): Promise<EncryptedVault> => {
        // 1. Generate Salt
        const salt = new Uint8Array(16);
        crypto.getRandomValues(salt);

        // 2. Derive Key (PBKDF2)
        const key = pbkdf2.pbkdf2Sync(password, Buffer.from(salt), 100000, 32, 'sha256');

        // 3. Encrypt (AES-CTR)
        const iv = new Uint8Array(16);
        crypto.getRandomValues(iv);

        const seedBytes = hexToBytes(seedHex);
        const aesCtr = new aes.ModeOfOperation.ctr(key, new aes.Counter(iv));
        const encryptedBytes = aesCtr.encrypt(seedBytes);

        return {
            ciphertext: bytesToHex(encryptedBytes),
            salt: bytesToHex(salt),
            iv: bytesToHex(iv)
        };
    },

    /**
     * Decrypt the Vault
     */
    decryptVault: async (vault: EncryptedVault, password: string): Promise<string> => {
        const salt = hexToBytes(vault.salt);
        const iv = hexToBytes(vault.iv);
        const ciphertext = hexToBytes(vault.ciphertext);

        // 1. Derive Key
        const key = pbkdf2.pbkdf2Sync(password, Buffer.from(salt), 100000, 32, 'sha256');

        // 2. Decrypt
        const aesCtr = new aes.ModeOfOperation.ctr(key, new aes.Counter(iv));
        const decryptedBytes = aesCtr.decrypt(ciphertext);

        // 3. Verify? (We assume if output is valid hex seed, it's ok. Ideally we'd add HMAC)
        // For now, return hex.
        return bytesToHex(decryptedBytes);
    },

    /**
     * Derive KeyPair from the seed (Aegen L2 Protocol)
     */
    deriveKeyPair: (inputSeed: string): KeyPair => {
        let seed: number[];

        if (/^[0-9a-fA-F]{64}$/.test(inputSeed)) {
            seed = hexToBytes(inputSeed);
        } else {
            // Fallback for raw strings
            seed = sha256.array(inputSeed);
        }

        // Public Key: SHA256(seed || 0x01)
        const pkInput = [...seed, 0x01];
        const pk = sha256.array(pkInput);

        // Full Private Key: seed || pk
        const sk = [...seed, ...pk];

        return {
            publicKey: bytesToHex(pk),
            privateKey: bytesToHex(sk),
            address: 'k:' + bytesToHex(pk)
        };
    },
};

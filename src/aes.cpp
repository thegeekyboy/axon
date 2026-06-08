/*
 * axon/aes.cpp  —  AES-128 ECB and CBC implementation
 *
 * REWRITE RATIONALE
 * -----------------
 * The original implementation used global mutable state (Key, RoundKey, Iv,
 * state) with no synchronisation, making every call data-race UB in a
 * multi-threaded programme.
 *
 * This rewrite delegates all cryptographic work to OpenSSL's EVP layer, which:
 *   • is fully thread-safe (no shared mutable globals)
 *   • uses hardware AES-NI automatically on any x86-64 / ARMv8 that has it
 *   • is FIPS-validated and receives ongoing security maintenance
 *   • is already a mandatory dependency of axon (libssl / libcrypto)
 *
 * The public API declared in axon/aes.h is preserved exactly:
 *
 *   ECB mode
 *	 void AES128_ECB_encrypt(uint8_t *input, const uint8_t *key, uint8_t *output)
 *	 void AES128_ECB_decrypt(uint8_t *input, const uint8_t *key, uint8_t *output)
 *
 *   CBC mode
 *	 void AES128_CBC_encrypt_buffer(uint8_t *output, uint8_t *input,
 *									uint32_t length, const uint8_t *key,
 *									const uint8_t *iv)
 *	 void AES128_CBC_decrypt_buffer(uint8_t *output, uint8_t *input,
 *									uint32_t length, const uint8_t *key,
 *									const uint8_t *iv)
 *
 * PARTIAL BLOCK CONTRACT (preserved from original)
 * -------------------------------------------------
 * When `length` is not a multiple of 16:
 *
 *   Encryption: the final partial block is zero-padded to 16 bytes before
 *   being encrypted.  The output buffer must therefore be large enough to
 *   hold ceil(length / 16) * 16 bytes.
 *
 *   Decryption: the caller must pass the same `length` that was used during
 *   encryption.  Internally the function rounds up to the nearest block
 *   boundary so that it decrypts all cipher blocks that were produced during
 *   encryption.  The output buffer must be at least ceil(length / 16) * 16
 *   bytes; only the first `length` bytes of the decrypted output are
 *   meaningful.
 *
 * BUILD
 * -----
 *   g++ -O2 -std=c++17 -c aes.cpp -o aes.o
 *   Link with: -lssl -lcrypto
 *
 * OpenSSL >= 1.1.0 is required (available on every RHEL/CentOS 7-9 that axon
 * already targets).
 */

#include <axon/aes.h>

#include <cstring>
#include <stdexcept>

#include <openssl/evp.h>
#include <openssl/err.h>

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {

constexpr int AES128_BLOCK  = 16;   // bytes per AES block
constexpr int AES128_KEYLEN = 16;   // bytes for a 128-bit key

// Throw a descriptive exception on OpenSSL failure.
[[noreturn]] void throw_openssl(const char *where)
{
	char buf[256];
	ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));
	throw std::runtime_error(std::string(where) + ": " + buf);
}

// RAII wrapper — never leak an EVP_CIPHER_CTX.
struct CipherCtx
{
	EVP_CIPHER_CTX *ctx;

	CipherCtx() : ctx(EVP_CIPHER_CTX_new())
	{
		if (!ctx) throw_openssl("EVP_CIPHER_CTX_new");
	}

	~CipherCtx() { EVP_CIPHER_CTX_free(ctx); }

	CipherCtx(const CipherCtx &) = delete;
	CipherCtx &operator=(const CipherCtx &) = delete;
};

// ---------------------------------------------------------------------------
// ECB — single 16-byte block
// ---------------------------------------------------------------------------

void ecb_crypt(const uint8_t *in, const uint8_t *key, uint8_t *out, int enc)
{
	CipherCtx c;

	if (EVP_CipherInit_ex(c.ctx, EVP_aes_128_ecb(), nullptr, key, nullptr, enc) != 1)
		throw_openssl("EVP_CipherInit_ex ECB");

	// Caller is responsible for padding — we operate on a single 16-byte block.
	EVP_CIPHER_CTX_set_padding(c.ctx, 0);

	int out_len = 0;
	if (EVP_CipherUpdate(c.ctx, out, &out_len, in, AES128_BLOCK) != 1)
		throw_openssl("EVP_CipherUpdate ECB");

	int final_len = 0;
	if (EVP_CipherFinal_ex(c.ctx, out + out_len, &final_len) != 1)
		throw_openssl("EVP_CipherFinal_ex ECB");
}

// ---------------------------------------------------------------------------
// CBC — arbitrary-length buffer
//
// Encryption: zero-pad final partial block then encrypt ceil(length/16) blocks.
// Decryption: decrypt ceil(length/16) blocks (same number that were produced
//			 during encryption).  `length` controls how many cipher blocks
//			 are read; the output buffer must hold ceil(length/16)*16 bytes.
// ---------------------------------------------------------------------------

void cbc_crypt(uint8_t *out, const uint8_t *in, uint32_t length, const uint8_t *key, const uint8_t *iv, int enc)
{
	if (length == 0) return;

	const uint32_t full_blocks = length / AES128_BLOCK;
	const uint32_t remainder_bytes = length % AES128_BLOCK;

	// Total number of cipher blocks to process (round up).
	const uint32_t total_blocks = full_blocks + (remainder_bytes ? 1 : 0);
	const uint32_t total_bytes = total_blocks * AES128_BLOCK;

	CipherCtx c;

	if (EVP_CipherInit_ex(c.ctx, EVP_aes_128_cbc(), nullptr, key, iv, enc) != 1)
		throw_openssl("EVP_CipherInit_ex CBC");

	// We manage padding ourselves to match the original API contract.
	EVP_CIPHER_CTX_set_padding(c.ctx, 0);

	int out_len = 0;

	if (enc == 1)
	{
		// --- ENCRYPT ---

		// Process all full blocks at once.
		if (full_blocks > 0)
		{
			if (EVP_CipherUpdate(c.ctx, out, &out_len, in, static_cast<int>(full_blocks * AES128_BLOCK)) != 1)
				throw_openssl("EVP_CipherUpdate CBC encrypt (full blocks)");
		}

		// Handle the final partial block: zero-pad into a staging buffer.
		if (remainder_bytes > 0)
		{
			uint8_t staging[AES128_BLOCK] = {};   // zero-initialised
			std::memcpy(staging, in + full_blocks * AES128_BLOCK, remainder_bytes);

			int partial_len = 0;
			if (EVP_CipherUpdate(c.ctx, out + out_len, &partial_len, staging, AES128_BLOCK) != 1)
				throw_openssl("EVP_CipherUpdate CBC encrypt (partial block)");
			out_len += partial_len;
		}
	}
	else
	{
		// --- DECRYPT ---
		//
		// We must decrypt `total_bytes` (rounded up) because that is how many
		// cipher bytes were produced during encryption.  The caller passes the
		// original plaintext length; we derive the correct cipher-byte count.

		if (EVP_CipherUpdate(c.ctx, out, &out_len, in, static_cast<int>(total_bytes)) != 1)
			throw_openssl("EVP_CipherUpdate CBC decrypt");
	}

	int final_len = 0;
	if (EVP_CipherFinal_ex(c.ctx, out + out_len, &final_len) != 1)
		throw_openssl("EVP_CipherFinal_ex CBC");
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Public API  (matches axon/aes.h exactly)
// ---------------------------------------------------------------------------

#if defined(ECB) && ECB

void AES128_ECB_encrypt(uint8_t *input, const uint8_t *key, uint8_t *output)
{
	ecb_crypt(input, key, output, 1);
}

void AES128_ECB_decrypt(uint8_t *input, const uint8_t *key, uint8_t *output)
{
	ecb_crypt(input, key, output, 0);
}

#endif // ECB

#if defined(CBC) && CBC

void AES128_CBC_encrypt_buffer(uint8_t *output, uint8_t *input, uint32_t length, const uint8_t *key, const uint8_t *iv)
{
	if (key == nullptr)
		throw std::invalid_argument("AES128_CBC_encrypt_buffer: key must not be null");

	cbc_crypt(output, input, length, key, iv, 1);
}

void AES128_CBC_decrypt_buffer(uint8_t *output, uint8_t *input, uint32_t length, const uint8_t *key, const uint8_t *iv)
{
	if (key == nullptr)
		throw std::invalid_argument("AES128_CBC_decrypt_buffer: key must not be null");

	cbc_crypt(output, input, length, key, iv, 0);
}

#endif // CBC


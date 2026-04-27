#include <windows.h>
#include <stdint.h>
#include <string.h>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <cstdint>
#include <immintrin.h>

// Custom HRESULTs from the assembly mapping
#define E_INVALID_KEY_FORMAT  ((HRESULT)0x80070057L) // E_INVALIDARG
#define E_LICENSE_TAMPERED    ((HRESULT)0x80041051L) // Custom License Error
#define BYTE_PTR(x) ((uint8_t*)(x))

// License Status Constants
#define LICENSE_VALID 1

/* --- IDA Pro Style Primitives --- */
typedef unsigned char      _BYTE;
typedef unsigned short     _WORD;
typedef unsigned int       _DWORD;
typedef unsigned __int64   _QWORD;

// Mapping __int128 for MSVC/GCC compatibility
#ifdef _MSC_VER
#include <intrin.h>
typedef __m128i _OWORD; // Using SIMD type for 128-bit storage in MSVC
#else
typedef __int128 _OWORD; // standard GCC/Clang extension
#endif

/* --- CONSTANTS & TABLES --- */

// Static Salt from .rdata:00000001800A6788
const uint8_t GEN4_SALT_DATA[16] = {
    0xEF, 0x72, 0x06, 0x66, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// .rdata:00000001800A67F0 - CRC32 Table
const _DWORD g_hash_table[256] = {
    0x00000000, 0x04C11DB7, 0x09823B6E, 0x0D4326D9, 0x130476DC, 0x17C56B6B, 0x1A864DB2, 0x1E475005,
    0x2608EDB8, 0x22C9F00F, 0x2F8AD6D6, 0x2B4BCB61, 0x350C9B64, 0x31CD86D3, 0x3C8EA00A, 0x384FBDBD,
    0x4C11DB70, 0x48D0C6C7, 0x4593E01E, 0x4152FDA9, 0x5F15ADAC, 0x5BD4B01B, 0x569796C2, 0x52568B75,
    0x6A1936C8, 0x6ED82B7F, 0x639B0DA6, 0x675A1011, 0x791D4014, 0x7DDC5DA3, 0x709F7B7A, 0x745E66CD,
    0x9823B6E0, 0x9CE2AB57, 0x91A18D8E, 0x95609039, 0x8B27C03C, 0x8FE6DD8B, 0x82A5FB52, 0x8664E6E5,
    0xBE2B5B58, 0xBAEA46EF, 0xB7A96036, 0xB3687D81, 0xAD2F2D84, 0xA9EE3033, 0xA4AD16EA, 0xA06C0B5D,
    0xD4326D90, 0xD0F37027, 0xDDB056FE, 0xD9714B49, 0xC7361B4C, 0xC3F706FB, 0xCEB42022, 0xCA753D95,
    0xF23A8028, 0xF6FB9D9F, 0xFBB8BB46, 0xFF79A6F1, 0xE13EF6F4, 0xE5FFEB43, 0xE8BCCD9A, 0xEC7DD02D,
    0x34867077, 0x30476DC0, 0x3D044B19, 0x39C556AE, 0x278206AB, 0x23431B1C, 0x2E003DC5, 0x2AC12072,
    0x128E9DCF, 0x164F8078, 0x1B0CA6A1, 0x1FCDBB16, 0x018AEB13, 0x054BF6A4, 0x0808D07D, 0x0CC9CDCA,
    0x7897AB07, 0x7C56B6B0, 0x71159069, 0x75D48DDE, 0x6B93DDDB, 0x6F52C06C, 0x6211E6B5, 0x66D0FB02,
    0x5E9F46BF, 0x5A5E5B08, 0x571D7DD1, 0x53DC6066, 0x4D9B3063, 0x495A2DD4, 0x44190B0D, 0x40D816BA,
    0xACA5C697, 0xA864DB20, 0xA527FDF9, 0xA1E6E04E, 0xBFA1B04B, 0xBB60ADFC, 0xB6238B25, 0xB2E29692,
    0x8AAD2B2F, 0x8E6C3698, 0x832F1041, 0x87EE0DF6, 0x99A95DF3, 0x9D684044, 0x902B669D, 0x94EA7B2A,
    0xE0B41DE7, 0xE4750050, 0xE9362689, 0xEDF73B3E, 0xF3B06B3B, 0xF771768C, 0xFA325055, 0xFEF34DE2,
    0xC6BCF05F, 0xC27DEDE8, 0xCF3ECB31, 0xCBFFD686, 0xD5B88683, 0xD1799B34, 0xDC3ABDED, 0xD8FBA05A,
    0x690CE0EE, 0x6DCDFD59, 0x608EDB80, 0x644FC637, 0x7A089632, 0x7EC98B85, 0x738AAD5C, 0x774BB0EB,
    0x4F040D56, 0x4BC510E1, 0x46863638, 0x42472B8F, 0x5C007B8A, 0x58C1663D, 0x558240E4, 0x51435D53,
    0x251D3B9E, 0x21DC2629, 0x2C9F00F0, 0x285E1D47, 0x36194D42, 0x32D850F5, 0x3F9B762C, 0x3B5A6B9B,
    0x0315D626, 0x07D4CB91, 0x0A97ED48, 0x0E56F0FF, 0x1011A0FA, 0x14D0BD4D, 0x19939B94, 0x1D528623,
    0xF12F560E, 0xF5EE4BB9, 0xF8AD6D60, 0xFC6C70D7, 0xE22B20D2, 0xE6EA3D65, 0xEBA91BBC, 0xEF68060B,
    0xD727BBB6, 0xD3E6A601, 0xDEA580D8, 0xDA649D6F, 0xC423CD6A, 0xC0E2D0DD, 0xCDA1F604, 0xC960EBB3,
    0xBD3E8D7E, 0xB9FF90C9, 0xB4BCB610, 0xB07DABA7, 0xAE3AFBA2, 0xAAFBE615, 0xA7B8C0CC, 0xA379DD7B,
    0x9B3660C6, 0x9FF77D71, 0x92B45BA8, 0x9675461F, 0x8832161A, 0x8CF30BAD, 0x81B02D74, 0x857130C3,
    0x5D8A9099, 0x594B8D2E, 0x5408ABF7, 0x50C9B640, 0x4E8EE645, 0x4A4FFBF2, 0x470CDD2B, 0x43CDC09C,
    0x7B827D21, 0x7F436096, 0x7200464F, 0x76C15BF8, 0x68860BFD, 0x6C47164A, 0x61043093, 0x65C52D24,
    0x119B4BE9, 0x155A565E, 0x18197087, 0x1CD86D30, 0x029F3D35, 0x065E2082, 0x0B1D065B, 0x0FDC1BEC,
    0x3793A651, 0x3352BBE6, 0x3E119D3F, 0x3AD08088, 0x2497D08D, 0x2056CD3A, 0x2D15EBE3, 0x29D4F654,
    0xC5A92679, 0xC1683BCE, 0xCC2B1D17, 0xC8EA00A0, 0xD6AD50A5, 0xD26C4D12, 0xDF2F6BCB, 0xDBEE767C,
    0xE3A1CBC1, 0xE760D676, 0xEA23F0AF, 0xEEE2ED18, 0xF0A5BD1D, 0xF464A0AA, 0xF9278673, 0xFDE69BC4,
    0x89B8FD09, 0x8D79E0BE, 0x803AC667, 0x84FBDBD0, 0x9ABC8BD5, 0x9E7D9662, 0x933EB0BB, 0x97FFAD0C,
    0xAFB010B1, 0xAB710D06, 0xA6322BDF, 0xA2F33668, 0xBCB4666D, 0xB8757BDA, 0xB5365D03, 0xB1F740B4
};

/* --- STRUCTURES --- */

#pragma pack(push, 1)
typedef union _DECODED_DATA {
    struct {
        _QWORD Low;         // 8 bytes (First half)
        _QWORD High;        // 8 bytes (Second half)
    } Half;                 // 2 x 8 bytes
    _BYTE Bytes[16];        // 16 x 1 byte (Array access)
} DECODED_DATA;

// Full Context Structure (96 bytes)
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _LICENSE_CONTEXT {
    uint8_t  Reserved0[8];      // 0x00
    uint8_t  MagicSalt[16];     // 0x08
    uint32_t RawSeedBits;       // 0x18
    uint32_t GroupID;           // 0x1C
    uint32_t Sequence;          // 0x20
    uint32_t ProductID;         // 0x24
    uint64_t KeyType;           // 0x28
    uint32_t InternalFlag;      // 0x30
    uint32_t State;             // 0x34
    uint8_t  TempWorkspace[32]; // 0x38 (covers offsets 56-87)
    uint8_t  HiddenGuard[8];    // 0x58? (optional for SSE safety)
} LICENSE_CONTEXT;
#pragma pack(pop)

// sub_180008DB4
__int64 __fastcall DecodeBase24(const wchar_t* szKey, __int64 a2, _BYTE* pOutBytes, __int64 a4, int* pOutSpecial) {
    unsigned int decodedCount = 0;
    unsigned int totalProcessed = 0;
    unsigned int hyphenCount = 0;
    int hasSpecialN = 0;

    while (decodedCount < 25) {
        wchar_t c = szKey[totalProcessed];

        // 1. Special 'N' Handling (ASCII 78)
        if (((c - 78) & 0xFFDF) == 0) {
            if (hasSpecialN || decodedCount >= 24) break;
            hasSpecialN = 1;
            memmove(pOutBytes + 1, pOutBytes, decodedCount);
            pOutBytes[0] = (_BYTE)decodedCount;
            goto NEXT_ITER;
        }

        // 2. Hyphen Validation
        if (c == L'-') {
            // 1. Declare as long (LONG)
            long mask = 8521760;

            // 2. Cast the index to long if totalProcessed is an unsigned int
            if (!_bittest(&mask, (long)totalProcessed) || hyphenCount >= 4) break;
            hyphenCount++;
            totalProcessed++;
            continue;
        }

        // 3. Alphabet Mapping (Verified via decompiler BST branches)
        _BYTE val;
        switch (c) {
        case L'B': val = 0;  break; case L'C': val = 1;  break;
        case L'D': val = 2;  break; case L'F': val = 3;  break;
        case L'G': val = 4;  break; case L'H': val = 5;  break;
        case L'J': val = 6;  break; case L'K': val = 7;  break;
        case L'M': val = 8;  break; case L'P': val = 9;  break;
        case L'Q': val = 10; break; case L'R': val = 11; break;
        case L'T': val = 12; break; case L'V': val = 13; break;
        case L'W': val = 14; break; case L'X': val = 15; break;
        case L'Y': val = 16; break; case L'2': val = 17; break;
        case L'3': val = 18; break; case L'4': val = 19; break;
        case L'6': val = 20; break; case L'7': val = 21; break;
        case L'8': val = 22; break; case L'9': val = 23; break;
        default: return 0x80040011; // Character not in Base24 set
        }

        pOutBytes[decodedCount] = val;

    NEXT_ITER:
        decodedCount++;
        totalProcessed++;

        if (totalProcessed >= 29) {
            if (hyphenCount == 4 && decodedCount == 25) {
                *pOutSpecial = hasSpecialN;
                return 0; // SUCCESS
            }
            break;
        }
    }

    return 0x80040011; // E_FAIL / INVALID_KEY
}

// sub_180008BCC
HRESULT ValidateAndUnswizzle(DECODED_DATA* pIn, uint8_t* pOut32)
{
    DECODED_DATA var_20 = *pIn;
    uint8_t var_40[16] = { 0 };
    uint8_t var_30[16] = { 0 };
    uint16_t var_50 = 0;

    // --- CRC / checksum
    uint8_t b12 = var_20.Bytes[12];
    uint8_t b13 = var_20.Bytes[13];
    uint8_t r8b = (uint8_t)(pIn->Half.High >> 0x30);
    var_50 = ((r8b << 1 | b13 >> 7) & 0x03) << 8 | ((b13 << 1) | (b12 >> 7));

    // --- Modify input bytes per assembly
    var_20.Bytes[12] &= 0x7F;
    var_20.Bytes[13] = 0;
    var_20.Bytes[14] = ((r8b & 0xF7) & 1) ^ (r8b & 0xF7);

    // --- CRC32 hash
    uint32_t ebx = 0xFFFFFFFF;
    for (int i = 0; i < 16; ++i) {
        uint8_t idx = (uint8_t)((ebx >> 24) ^ var_20.Bytes[i]);
        ebx = (ebx << 8) ^ g_hash_table[idx];
    }
    ebx = (~ebx) & 0x3FF;
    if (var_50 != (uint16_t)ebx) return 0x8007000D;

    // --- Unsizzle
    // 1. Direct copy first 2 bytes
    *(uint16_t*)&var_40[0] = *(uint16_t*)&var_20.Bytes[0];

    // 2. var_40[2] pivot
    var_40[2] = var_20.Bytes[2] & 0x0F;

    // 3. Expansion Loop 1
    for (int i = 0; i < 3; i++)
        var_40[i + 4] = (var_20.Bytes[i + 2] >> 4) | (var_20.Bytes[i + 3] << 4);

    // 4. Byte 7 masking
    uint8_t b7_bits = (var_20.Bytes[6] << 4) | (var_20.Bytes[5] >> 4);
    var_40[7] = (var_40[7] & 0xC0) | (b7_bits & 0x3F);

    // 5. Expansion Loop 2 (var_30)
    for (int i = 0; i < 6; i++)
        var_30[i] = (var_20.Bytes[i + 6] >> 2) | (var_20.Bytes[i + 7] << 6);

    // 6. Final cleanups
    var_30[6] = (var_20.Bytes[12] >> 2) & 0x1F;
    var_40[8] = (var_20.Bytes[14] >> 1) & 0x01;

    // 7. Copy output
    memcpy(pOut32, var_40, 16);
    memcpy(pOut32 + 16, var_30, 16);

    return S_OK;
}

// sub_18000890C
HRESULT __fastcall CompressKey(const wchar_t* szKey, DECODED_DATA* pOutRaw, int* pOutFlag)
{
    // local variables to match ASM stack
    int specialNFlag = 0;                // var_68
    uint8_t decodedBytes[32] = { 0 };    // var_50 (v22 in pseudocode)
    uint8_t accum[16] = { 0 };           // var_60
    uint32_t ebx_status;

    if (!szKey) return 0x80041051;

    // 1. Length Check (Exactly 29 characters)
    size_t len = 0;
    while (len < 30 && szKey[len] != 0) len++;
    if (len != 29) return 0x80041051;

    // 2. Base24 Decoding 
    // Matching ASM: call sub_180008DB4(rcx: szKey, rdx: 29i64, r8: decodedBytes)
    // Note: The assembly passes '29' as the second argument.
    // Adjust this call to match your DecodeBase24 signature.
    ebx_status = (uint32_t)DecodeBase24(szKey, 29, decodedBytes, 0, &specialNFlag);

    if ((int)ebx_status < 0) {
        return ebx_status;
    }

    // 3. BigInt Conversion (Base24 -> Bytes)
    uint32_t currentByteCount = 0;

    for (int i = 0; i < 25; ++i) {
        uint32_t carry = decodedBytes[i];

        if (currentByteCount > 0) {
            for (uint32_t j = 0; j < currentByteCount; ++j) {
                uint32_t temp = (uint32_t)accum[j] * 24 + carry;
                accum[j] = (uint8_t)temp;
                carry = temp >> 8;
            }
        }

        if (carry != 0) {
            if (currentByteCount >= 16) return 0x80041051;
            accum[currentByteCount] = (uint8_t)carry;
            currentByteCount++;
        }
    }

    // 4. Handle the "N" Flag logic
    // Assembly: mov eax, [rsp+98h+var_68] -> test eax, eax
    // This flag is usually set inside sub_180008DB4 if it's a specific key type
    int finalFlagValue = 0;
    if (specialNFlag != 0) {
        accum[14] |= 0x08; // Set the special bit in Byte 14
        finalFlagValue = 1;
    }

    // 5. Final Move
    memcpy(pOutRaw, accum, 16);

    if (pOutFlag) {
        *pOutFlag = finalFlagValue;
    }

    return ebx_status;
}

// sub_180008A58
__int64 __fastcall UnpackLicenseContext(unsigned char* pDecodedKey, byte* pContext)
{
    // var_20 is an 8-byte qword, var_17 starts right after it, totaling 16 bytes of scratch space.
    uint8_t tmp[16];
    memset(tmp, 0, 16);

    // .text:180008A7E - cmp [rcx+8], eax / setnz byte ptr [rbp+var_20]
    // Sets the first byte of tmp to 1 if the unswizzled data at offset 8 is non-zero.
    tmp[0] = (*(uint32_t*)(pDecodedKey + 8) != 0) ? 1 : 0;

    // --- LOOP 1: Shift Left 1 (.text:180008AA7) ---
    // This loop takes 3 bytes starting at pDecodedKey[4] and merges them into tmp[0-3]
    for (int i = 0; i < 3; ++i) {
        uint8_t val = pDecodedKey[i + 4];
        tmp[i] = (tmp[i] & 1) | (uint8_t)(val << 1);    // or [r8], al (al = cl + cl)
        tmp[i + 1] = (uint8_t)(val >> 7);               // or [r9], cl (cl >> 7)
    }

    // --- THE PIVOT: Byte 3 (.text:180008ACC) ---
    // Specifically modifies the 4th byte of the scratch buffer using pDecodedKey[7]
    uint8_t key7 = pDecodedKey[7];
    uint8_t current3 = tmp[3];
    // .text:180008AE0 - xor cl, al / and cl, 7Eh / xor al, cl
    // This is a bitwise "mask move": tmp[3] bits 1-6 come from (key7 << 1)
    tmp[3] = current3 ^ ((current3 ^ (key7 << 1)) & 0x7E);

    // --- LOOP 2: Shift Left 7 (.text:180008AF7) ---
    // Processes 2 bytes starting at pDecodedKey[3] (yes, it goes backward in the key)
    for (int i = 0; i < 2; ++i) {
        uint8_t val = pDecodedKey[i + 3];
        tmp[i + 3] = (tmp[i + 3] & 0x7F) | (uint8_t)(val << 7);
        tmp[i + 4] = (tmp[i + 4] & 0x80) | (uint8_t)(val >> 1);
    }

    // --- METADATA: Byte 2 (.text:180008B1A) ---
    uint8_t key2 = pDecodedKey[2];
    // tmp[5] high bit comes from key2 low bit
    tmp[5] = (tmp[5] & 0x7F) | (key2 << 7);
    // tmp[6] low 3 bits come from key2 bits 1-3
    tmp[6] = tmp[6] ^ ((tmp[6] ^ (key2 >> 1)) & 7);

    // --- LOOP 3: Shift Left 3 (.text:180008B5B) ---
    // Processes 6 bytes starting at pDecodedKey[16]
    for (int i = 0; i < 6; ++i) {
        uint8_t val = pDecodedKey[i + 16];
        tmp[i + 6] = (tmp[i + 6] & 7) | (uint8_t)(val << 3);
        tmp[i + 7] = (tmp[i + 7] & 0xF8) | (uint8_t)(val >> 5);
    }

    // --- FINAL PACKING (.text:180008B86) ---
    // Writes the results into the provided LICENSE_CONTEXT structure

    // Offset 0x00: First 8 bytes (tmp[0] to tmp[7])
    *(uint64_t*)((uint8_t*)pContext + 0x00) = *(uint64_t*)&tmp[0];

    // Offset 0x08: Next 4 bytes (tmp[8] to tmp[11])
    *(uint32_t*)((uint8_t*)pContext + 0x08) = *(uint32_t*)&tmp[8];

    // Offset 0x0C: Final byte 
    // .text:180008B7F - mov cl, [r11+6] (r11 was key+0x10, so key[0x16] = 22)
    uint8_t key22 = pDecodedKey[22];
    uint8_t tmp12 = tmp[12] & 7;
    ((uint8_t*)pContext)[0x0C] = (key22 << 3) | tmp12;

    // Cleanup call

    return 0;
}

// KeyInfo. BOB [MDL]
bool GetInfo_Gen4(const uint8_t* key3, LICENSE_CONTEXT* pContext) {
    if (!key3 || !pContext) return false;

    // Bob's GroupId extraction (Bits 0-19)
    uint32_t groupid = 0;
    groupid |= key3[0];
    groupid |= (uint32_t)key3[1] << 8;
    groupid |= (uint32_t)(key3[2] & 0x0F) << 16;
    pContext->GroupID = groupid;

    // Bob's KeyId extraction (Bits 20-49)
    uint32_t keyid = 0;
    keyid |= (key3[2] >> 4);
    keyid |= (uint32_t)key3[3] << 4;
    keyid |= (uint32_t)key3[4] << 12;
    keyid |= (uint32_t)key3[5] << 20;
    keyid |= (uint32_t)(key3[6] & 0x3F) << 28; // Masking at 30 bits
    pContext->Sequence = keyid;

    // Bob's Secret extraction (Bits 50-113)
    uint64_t secret = 0;
    secret |= (key3[6] >> 2);
    secret |= (uint64_t)key3[7] << 6;
    secret |= (uint64_t)key3[8] << 14;
    secret |= (uint64_t)key3[9] << 22;
    secret |= (uint64_t)key3[10] << 30;
    secret |= (uint64_t)key3[11] << 38;
    secret |= (uint64_t)(key3[12] & 0x1F) << 46;
    pContext->KeyType = secret; // Mapping Secret to KeyType for your struct

    return true;
}

// sub_1800090B0
HRESULT DecodeLicenseContext(const wchar_t* ProductKey, LICENSE_CONTEXT* Context, BOOLEAN UseBob)
{
    if (!Context) return E_POINTER;
    if (!ProductKey) return E_INVALIDARG;

    uint8_t buffer[96];
    memset(buffer, 0, sizeof(buffer));

    uint8_t unswizzled[32];
    memset(unswizzled, 0, sizeof(unswizzled));

    int isKeyValid = 0;
    HRESULT hr = S_OK;

    // 1. COMPRESS
    hr = CompressKey(ProductKey, (DECODED_DATA*)&buffer[64], &isKeyValid);
    if (FAILED(hr)) return hr;

    *(uint32_t*)&buffer[0x30] = 1; // InternalFlag

    if (isKeyValid)
    {
        if (UseBob)
            return GetInfo_Gen4(&buffer[64], Context);

        // 2. UNSWIZZLE
        hr = ValidateAndUnswizzle((DECODED_DATA*)&buffer[64], unswizzled);
        if (hr == (HRESULT)0x8007000D) return (HRESULT)0x80041111;
        if (FAILED(hr)) return hr;

        // 3. UNPACK
        hr = UnpackLicenseContext(unswizzled, &buffer[72]);
        if (FAILED(hr)) return hr;

        // 4. LOAD KEYTYPE
        uint64_t keyType = *(uint64_t*)&unswizzled[16];
        *(uint64_t*)&buffer[0x28] = keyType;

        // Salt + state
        memcpy(&buffer[0x08], GEN4_SALT_DATA, 16);
        *(uint32_t*)&buffer[0x34] = 1;

        // =========================
        // 🔥 FIXED MATH BLOCK
        // =========================

        uint32_t combinedID = *(uint32_t*)&unswizzled[4]; // 32-bit field

        // Multiply like 'mul ecx' in x86
        uint8_t* key3 = &buffer[64]; // 32 bytes
        uint64_t result = (uint64_t)combinedID * 0x431BDE83ULL;
        uint32_t edx = (uint32_t)(result >> 32);
        uint32_t sequence = combinedID - ((edx >> 18) * 1000000);

        // From key info tool By Bob [MDL]
        uint32_t groupID = key3[0] | (key3[1] << 8) | ((key3[2] & 0x0F) << 16);

        // Write back
        *(uint32_t*)&buffer[0x1C] = groupID;
        *(uint32_t*)&buffer[0x20] = sequence;

        // =========================

        *(uint32_t*)&buffer[0x24] = *(uint32_t*)&unswizzled[8]; // ProductID
        *(uint32_t*)&buffer[0x18] = *(uint32_t*)&unswizzled[0]; // RawSeedBits
    }

    memcpy(Context, buffer, sizeof(LICENSE_CONTEXT));
    return hr;
}

int main() {
    // A sample key format (This must be a valid Base24 key for your specific app)
    const wchar_t* myKey = L"K8KNG-MGG4H-KX82M-M8QYW-DGRFH";

    LICENSE_CONTEXT ctx = { 0 };

    /*
        Key      : K8KNG-MGG4H-KX82M-M8QYW-DGRFH
        Integer  : 4441847703199715836355246925287693
        Group    : 4365
        Serial   : 11
        Security : 12
        Checksum : 438
        Upgrade  : 0
        Extra    : 0
    */

    std::cout << "--- License Decoder Tool ---" << std::endl;
    std::cout << "Testing Key: " << std::hex << myKey << std::endl;

    HRESULT hr = DecodeLicenseContext(myKey, &ctx, 0);

    if (SUCCEEDED(hr)) {
        std::cout << "Result:  VALID" << std::endl;
        std::cout << "----------------------------" << std::endl;
        std::cout << "Product ID: " << std::dec << ctx.ProductID << std::endl;
        std::cout << "Group ID:   " << ctx.GroupID << std::endl;
        std::cout << "Sequence:   " << ctx.Sequence << std::endl;
        std::cout << "Key Type:   0x" << std::hex << ctx.KeyType << std::endl;
        std::cout << "State:      " << std::dec << ctx.State << std::endl;
    }
    else {
        std::cerr << "Result:  INVALID (HRESULT: 0x" << std::hex << hr << ")" << std::endl;
        if (hr == E_LICENSE_TAMPERED) {
            std::cerr << "Reason:  Checksum mismatch (CRC failed)." << std::endl;
        }
        else if (hr == 0x80040011) {
            std::cerr << "Reason:  Invalid characters or length." << std::endl;
        }
    }

    system("pause");
    return 0;
}
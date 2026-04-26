
/* Tsforge .. Source .. */

public string GetPid2()
{
	if (pid2 != null)
	{
		return pid2;
	}

	pid2 = "";

	if (Algorithm == PKeyAlgorithm.PKEY2005)
	{
		string mpc = GetMPC();
		string serialHigh;
		int serialLow;
		int lastPart;

		if (EulaType == "OEM")
		{
			serialHigh = "OEM";
			serialLow = ((Group / 2) % 100) * 10000 + (Serial / 100000);
			lastPart = Serial % 100000;
		}
		else
		{
			serialHigh = (Serial / 1000000).ToString("D3");
			serialLow = Serial % 1000000;
			lastPart = ((Group / 2) % 100) * 1000 + new Random().Next(1000);
		}

		int checksum = 0;

		foreach (char c in serialLow.ToString())
		{
			checksum += int.Parse(c.ToString());
		}
		checksum = 7 - (checksum % 7);

		pid2 = string.Format("{0}-{1}-{2:D6}{3}-{4:D5}", mpc, serialHigh, serialLow, checksum, lastPart);
	}

	return pid2;
}

public byte[] GetPid3()
{
	BinaryWriter writer = new BinaryWriter(new MemoryStream());
	writer.Write(0xA4);
	writer.Write(0x3);
	writer.WriteFixedString(GetPid2(), 24);
	writer.Write(Group);
	writer.WriteFixedString(PartNumber, 16);
	writer.WritePadding(0x6C);
	byte[] data = writer.GetBytes();
	byte[] crc = BitConverter.GetBytes(~Utils.CRC32(data.Reverse().ToArray())).Reverse().ToArray();
	writer.Write(crc);

	return writer.GetBytes();
}

public byte[] GetPid4()
{
	BinaryWriter writer = new BinaryWriter(new MemoryStream());
	writer.Write(0x4F8);
	writer.Write(0x4);
	writer.WriteFixedString16(GetExtendedPid(), 0x80);
	writer.WriteFixedString16(ActivationId.ToString(), 0x80);
	writer.WritePadding(0x10);
	writer.WriteFixedString16(Edition, 0x208);
	writer.Write(Upgrade ? (ulong)1 : 0);
	writer.WritePadding(0x50);
	writer.WriteFixedString16(PartNumber, 0x80);
	writer.WriteFixedString16(Channel, 0x80);
	writer.WriteFixedString16(EulaType, 0x80);

	return writer.GetBytes();
}

public string GetExtendedPid()
{
	string mpc = GetMPC();
	int serialHigh = Serial / 1000000;
	int serialLow = Serial % 1000000;
	int licenseType;
	uint lcid = Utils.GetSystemDefaultLCID();
	int build = Environment.OSVersion.Version.Build;
	int dayOfYear = DateTime.Now.DayOfYear;
	int year = DateTime.Now.Year;

	switch (EulaType)
	{
		case "OEM":
			licenseType = 2;
			break;

		case "Volume":
			licenseType = 3;
			break;

		default:
			licenseType = 0;
			break;
	}

	return string.Format(
		"{0}-{1:D5}-{2:D3}-{3:D6}-{4:D2}-{5:D4}-{6:D4}.0000-{7:D3}{8:D4}",
		mpc,
		Group,
		serialHigh,
		serialLow,
		licenseType,
		lcid,
		build,
		dayOfYear,
		year
	);
}

/**
 * Origin: Sppobjs.dll
 * __int64 __fastcall CProductKeyUtilsT<CEmptyType>::BinaryEncode(__m128i *a1, __int64 a2, unsigned __int16 **a3)
 */
 
 // MSVC doesn't support __int128 natively. Use a struct to match __m128i.
struct u128 {
    unsigned char data[16];
};

// Original Name: sub_18013CC20
NTSTATUS EncodeProductKey(const wchar_t* key, u128* outBin, DWORD* outType) {
    if (!key || !outBin) return 0xC000000D;

    // Zero out the buffer
    memset(outBin->data, 0, 16);

    int nPos = -1, count = 0;
    const wchar_t* charset = L"BCDFGHJKMPQRTVWXY2346789";

    for (int i = 0; key[i] && count < 25; i++) {
        if (key[i] == L'-') continue;
        if (key[i] == L'N') { nPos = count; continue; }

        int val = -1;
        for (int j = 0; j < 24; j++) if (charset[j] == key[i]) val = j;
        if (val == -1) return 0xC0000001;

        unsigned int carry = val;
        for (int j = 0; j < 16; j++) {
            unsigned int tmp = outBin->data[j] * 24 + carry;
            outBin->data[j] = (unsigned char)(tmp & 0xFF);
            carry = tmp >> 8;
        }
        count++;
    }

    if (nPos != -1) {
        // Modern Key logic: sub_18013D1B4
        outBin->data[14] = (outBin->data[14] & 0xF7) | 0x08;
        outBin->data[15] = (unsigned char)nPos;
    }

    if (outType) *outType = (nPos != -1);
    return 0;
}
 
 /**
 * Origin: Sppobjs.dll
 * __int64 __fastcall CProductKeyUtilsT<CEmptyType>::BinaryDecode(__m128i *a1)
 */
 
// MSVC doesn't support __int128 natively. Use a struct to match __m128i.
struct u128 {
    unsigned char data[16];
};

// Original Name: sub_18013C8DC
std::wstring DecodeProductKey(u128 bin) {
    unsigned char bytes[16];
    memcpy(bytes, bin.data, 16);

    bool hasN = (bytes[14] & 0x08) != 0;
    int nPos = bytes[15];
    bytes[14] &= 0xF7;
    bytes[15] = 0;

    wchar_t raw[27] = { 0 }; // Extra space for safety
    const wchar_t* alphabet = L"BCDFGHJKMPQRTVWXY2346789";

    for (int i = 24; i >= 0; i--) {
        unsigned int rem = 0;
        for (int j = 15; j >= 0; j--) {
            unsigned int val = bytes[j] + (rem << 8);
            bytes[j] = (unsigned char)(val / 24);
            rem = val % 24;
        }
        raw[i] = alphabet[rem];
    }

    std::wstring res(raw);
    if (hasN) {
        res.erase(0, 1);
        res.insert(nPos, 1, L'N');
    }

    std::wstring formatted = L"";
    for (int i = 0; i < 25; i++) {
        if (i > 0 && i % 5 == 0) formatted += L'-';
        formatted += res[i];
    }
    return formatted;
}

// Original from sppwinob.dll, Windws 8 Beta 7850
// Windows 8 Build 7850 Symbols (x64 Fre)
// https://archive.org/details/Win8_7850_x64fre_symbols

__int64 __fastcall CProductKeyUtilsT<CEmptyType>::BinaryDecode(__int128 *a1, unsigned __int16 **a2)
{
  unsigned __int16 *v2; // rbx
  int v5; // edi
  __int64 v6; // r9
  BOOL v7; // r10d
  __int64 v8; // r8
  __int64 i; // rcx
  unsigned int v10; // r8d
  __int64 v11; // rbx
  int v12; // eax
  unsigned __int16 *v13; // r8
  int v14; // r9d
  __int64 v15; // r10
  int v16; // edx
  __int16 v17; // ax
  __int64 v18; // rcx
  HANDLE ProcessHeap; // rax
  unsigned __int16 *v21; // [rsp+28h] [rbp-51h] BYREF
  __int128 v22; // [rsp+30h] [rbp-49h]
  __int16 v23; // [rsp+40h] [rbp-39h] BYREF
  __int16 Src[27]; // [rsp+42h] [rbp-37h] BYREF
  __int16 v25[28]; // [rsp+78h] [rbp-1h] BYREF

  v2 = 0i64;
  v21 = 0i64;
  memcpy(v25, L"BCDFGHJKMPQRTVWXY2346789", 0x32ui64);
  v22 = *a1;
  if ( (BYTE14(v22) & 0xF0) != 0 )
    goto LABEL_2;
  v6 = 24i64;
  v7 = (BYTE14(v22) & 8) != 0;
  BYTE14(v22) = (4 * (((BYTE14(v22) & 8) != 0) & 2)) | BYTE14(v22) & 0xF7;
  do
  {
    LODWORD(v8) = 0;
    for ( i = 14i64; i >= 0; --i )
    {
      v10 = *((unsigned __int8 *)&v22 + i) + ((_DWORD)v8 << 8);
      *((_BYTE *)&v22 + i) = v10 / 0x18;
      v8 = v10 % 0x18;
    }
    Src[--v6] = v25[v8];
  }
  while ( v6 >= 0 );
  if ( (_BYTE)v22 )
  {
LABEL_2:
    v5 = -2147024883;
    CBreakOnFailureT<CEmptyType>::CheckToBreakOnFailure(2147942413i64);
LABEL_16:
    if ( WPP_GLOBAL_Control != &WPP_GLOBAL_Control && (*((_BYTE *)WPP_GLOBAL_Control + 68) & 2) != 0 )
      WARBIRD::g_IndirectBranchTargets8(
        *((_QWORD *)WPP_GLOBAL_Control + 7),
        11i64,
        WPP_33b221283fe149b69b56c7e3db1fdc6a_Traceguids,
        (unsigned int)v5);
    goto LABEL_19;
  }
  if ( v7 )
  {
    v11 = v8;
    memmove(&v23, Src, 2 * v8);
    Src[v11 - 1] = 78;
  }
  v12 = STRAPI_CreateCchBufferN(0x2Du, 0x1Eui64, &v21);
  v5 = v12;
  if ( v12 >= 0 )
  {
    v13 = v21;
    v14 = 0;
    v15 = 0i64;
    do
    {
      v16 = (unsigned __int64)(1717986919i64 * v14++) >> 32;
      v17 = Src[v15 - 1];
      v18 = v15 + (int)(((unsigned int)v16 >> 31) + (v16 >> 1));
      ++v15;
      v13[v18] = v17;
    }
    while ( v14 < 25 );
    v2 = 0i64;
    *a2 = v13;
  }
  else
  {
    CBreakOnFailureT<CEmptyType>::CheckToBreakOnFailure((unsigned int)v12);
    v2 = v21;
  }
  if ( v5 < 0 )
    goto LABEL_16;
LABEL_19:
  if ( v2 )
  {
    ProcessHeap = GetProcessHeap();
    HeapFree(ProcessHeap, 0, v2 - 2);
  }
  return (unsigned int)v5;
}

/**
 * Origin: Sppobjs.dll
 * Logic: Serializes license data into the legacy 164-byte Product ID structure.
 */

// Original Name: sub_18013BA14
NTSTATUS BuildDigitalProductId3(Context* ctx, HWND hWnd, void* reserved, Pid3Buffer* output)
{
    NTSTATUS status;
    wchar_t formattedPid[48]; // Local stack buffer for the PID string

    // 1. Validation: Verify the buffer is specifically sized for PID3 (164 bytes)
    if ( output->TotalSize != 164 ) 
    {
        status = STATUS_INVALID_PARAMETER; // 0x80070057
        sub_18008A114(status);             // Log Error
        return status;
    }

    // 2. Setup: Clear data area and set Header version to 3
    memset(output->Data, 0, 156); 
    output->TotalSize = 164;
    output->Version = 3;

    // 3. String Generation: Call the Algorithm Dispatcher (Sppobjs.sub_18013BBB4)
    // This decides if the PID is modern PKEY 2009 or Legacy Mod7.
    status = sub_18013BBB4(ctx, hWnd, 0, formattedPid);
    if ( status < 0 ) goto Exit;

    // 4. Data Packing: Fill the binary slots
    // Copy the PID string (e.g., "00330-80000-...") to Offset 8
    status = sub_18013C5F8(output->PidString, 24, formattedPid);
    if ( status < 0 ) goto Exit;

    // Set Group ID from Algorithm Context
    output->GroupId = ctx->PKeyAlgorithm->GroupId; // Offset 32

    // Copy Part Number from License Properties
    status = sub_18013C5F8(output->PartNumber, 16, ctx->License->PartNumber); // Offset 36
    if ( status < 0 ) goto Exit;

    // 5. Metadata Mapping
    output->ProductKeyID = ctx->KeyGuid;           // Offset 52
    sub_18013B9D0(ctx->InstallDate, output->Date); // Offset 72 (Day/Year)
    output->KeyType = ctx->KeyTypeFlag;            // Offset 76

    // 6. Channel ID: Determine OEM (2) vs Retail (3) vs Volume (sub_18013C75C)
    status = sub_18013C75C(ctx, &output->ChannelType); // Offset 80
    if ( status < 0 ) goto Exit;

    // 7. Hardware Binding: Copy 8-byte Hardware seed
    status = sub_18013C5F8(output->HardwareID, 8, &GlobalHardwareSeed); // Offset 92
    if ( status < 0 ) goto Exit;

    // 8. Integrity Seal: Finalize with a 4-byte Checksum (sub_18013B974)
    // Calculates CRC of first 160 bytes and writes result to Offset 160.
    sub_18013B974(output, 0, output->Checksum);

Exit:
    sub_18008D904(status); // Clean up / Return Status
    return status;
}

/**
 * Origin: Sppobjs.dll
 * Logic: Scans license properties to classify the distribution channel.
 */

// Original Name: sub_18013C75C
NTSTATUS IdentifyLicenseChannel(Context* ctx, DWORD* outChannelType)
{
    NTSTATUS status;
    BOOL isFound = FALSE; // This corresponds to the stack variable v8

    // 1. Check for "OEM" Channel
    // ctx + 120 points to the parsed XML license property bag
    status = SearchLicenseProperty(ctx->PropertyBag, L"OEM", &isFound);
    
    if (status < 0) goto Error; // Search failed (e.g. out of memory)

    // If "OEM" keyword was found in the license
    if (isFound) 
    {
        *outChannelType = 2; // Magic Number for OEM
        return STATUS_SUCCESS;
    }

    // 2. Check for "Volume" Channel
    status = SearchLicenseProperty(ctx->PropertyBag, L"Volume", &isFound);
    if (status >= 0) 
    {
        // 3. Check for "Retail" Channel
        status = SearchLicenseProperty(ctx->PropertyBag, L"Retail", &isFound);
        if (status >= 0) 
        {
            // 4. Check for "VT" (Volume Trial / Validation Type)
            status = SearchLicenseProperty(ctx->PropertyBag, L"VT", &isFound);
            
            if (status >= 0) 
            {
                // If we went through all keywords and found NOTHING
                status = 0x89FA0001; // Error: Unknown License Channel
                goto Error;
            }
        }
    }

Error:
    LogError(status);    // sub_18008A114
    Cleanup(status);    // sub_18008D904
    return status;
}

/**
 * Origin: Sppobjs.dll
 * Logic: Formats the legacy 20-character Product ID based on license channel.
 */

// Original Name: sub_18013BC64
NTSTATUS FormatLegacyProductId(Context* ctx, wchar_t* mpc, wchar_t* outputBuffer)
{
    NTSTATUS status;
    int channelType; // Received from IdentifyLicenseChannel (v13)
    DWORD serial;
    DWORD dateCode;
    DWORD groupId;
    DWORD checksum;

    // 1. Identify the license channel (OEM, Retail, Volume)
    status = IdentifyLicenseChannel(ctx, &channelType); // sub_18013C75C
    if (status < 0) goto Exit;

    // 2. Branching Logic based on Channel
    if (channelType != 2) // --- RETAIL / VOLUME PATH ---
    {
        dateCode = ctx->DateCode % 1000;              // Extract build/date segment
        serial   = ctx->SerialNumber % 1000000;       // Extract 6-digit serial
        checksum = CalculateMod7(serial);             // sub_18013BB5C (Mod7 check)
        
        groupId  = (ctx->KeyGroupId % 1000) + (1000 * ((ctx->Header->Flag >> 1) % 100));

        // Format: "MPC-Date-SerialSum-GroupId" (e.g., 00330-123-1234567-12345)
        status = SafePrintf(outputBuffer, 0x30, L"%05.5s-%03.3lu-%07.7lu-%05.5lu", 
                            mpc, dateCode, checksum, groupId);
    }
    else // --- OEM PATH ---
    {
        serial = ctx->SerialNumber;                   // Raw serial (v8)
        
        // Complex calculation mixing date, flags, and serial for the OEM segment
        DWORD oemInput = (serial % 1000000 / 100000) + 
                         10 * ((ctx->DateCode % 1000) + 1000 * ((ctx->Header->Flag >> 1) % 100));
        
        checksum = CalculateMod7(oemInput);           // sub_18013BB5C
        DWORD lowSerial = serial % 100000;            // Last 5 digits

        // Format: "MPC-OEM-SerialSum-LowSerial" (e.g., 00330-OEM-1234567-12345)
        status = SafePrintf(outputBuffer, 0x30, L"%05.5s-OEM-%07.7lu-%05.5lu", 
                            mpc, checksum, lowSerial);
    }

Exit:
    if (status < 0) LogError(status); // sub_18008A114
    Cleanup(status);                  // sub_18008D904
    return status;
}

/**
 * Origin: Sppobjs.dll
 * Logic: Formats the high-precision Extended Product ID (EPID).
 */

// Original Name: sub_18013BFA8
NTSTATUS FormatExtendedProductId(Context* ctx,  wchar_t* mpc, int languageId, int osVersion, int mysteryParam, wchar_t* outputBuffer)
{
    NTSTATUS status;
    DWORD channelType;  // OEM, Retail, etc.
    DWORD dayOfYear;    // 1-366
    DWORD year;         // e.g., 2026
    
    // 1. Identify License Channel (sub_18013C75C)
    status = IdentifyLicenseChannel(ctx, &channelType);
    if (status < 0) goto Error;

    // 2. Calculate Installation Date Info (sub_18013C654)
    // Converts ctx + 16 (FILETIME) to Day of Year and Year
    status = CalculateActivationDate(ctx->InstallTimestamp, &dayOfYear, &year);
    if (status < 0) goto Error;

    // 3. Prepare Buffer
    memset(outputBuffer, 0, 128); // Clear 128-byte buffer

    // 4. Data Extraction using Modulo (packing values into specific widths)
    DWORD groupId  = ctx->AlgorithmInfo->GroupId % 100000; // v13
    DWORD buildNum = ctx->BuildNumber % 1000;             // v14
    DWORD serial   = ctx->SerialNumber % 1000000;         // v15

    // 5. Final String Assembly
    // Template: "MPC-Group-Build-Serial-Channel-Lang-OS-Unknown-DayYear"
    // Example: "00330-10000-000-123456-02-1033-19044.0000-1112026"
    status = SafePrintf(
        outputBuffer,
        128,
        L"%05.5s-%05.5u-%03.3u-%06.6u-%02.2u-%04.4u-%04.4u.%04.4u-%03.3u%04.4u",
        mpc,            // %05.5s
        groupId,        // %05.5u
        buildNum,       // %03.3u
        serial,         // %06.6u
        channelType,    // %02.2u
        languageId,     // %04.4u
        osVersion,      // %04.4u
        mysteryParam,   // %04.4u
        dayOfYear,      // %03.3u
        year            // %04.4u
    );

    if (status >= 0) return status;

Error:
    LogError(status);    // sub_18008A114
    Cleanup(status);     // sub_18008D904
    return status;
}

/**
 * Origin: Sppobjs.dll
 * Logic: Formats the Product ID using the PKEY 2009 algorithm template.
 */

// Original Name: sub_18013BE18
NTSTATUS FormatModernProductId(int channelType, unsigned int groupId, unsigned int serial, unsigned int build, int isGenuine, wchar_t* outputBuffer)
{
    NTSTATUS status;
    
    // 1. Numerical Decomposition
    // Breaks the serial and group IDs into specific segments for the string
    unsigned int serialHigh = serial / 100000;         // v7
    unsigned int serialMid  = serial % 100000;         // v8 (Last 5 digits)
    unsigned int serialCheck = serialHigh % 10000;     // v9
    unsigned int groupSegment = (groupId / 10) % 100000; // v10

    // 2. Character Generation
    // Generates 'A' or 'B' based on the Genuine flag (65 is ASCII 'A')
    int char1 = 65;                    // 'A'
    int char2 = (isGenuine != 0) + 65; // 'A' if genuine, 'B' if not

    // 3. Format Branching
    if (channelType == 2) // --- OEM PATH ---
    {
        unsigned int groupDigit = groupId % 10;

        // Template: "Group-DigitSerial-Serial-AAOEM"
        // Example: "00330-01234-56789-AAOEM"
        status = SafePrintf(
            outputBuffer, 
            48, 
            L"%05.5lu-%01.1lu%04.4lu-%05.5lu-%c%cOEM", 
            groupSegment, 
            groupDigit, 
            serialCheck, 
            serialMid, 
            char1, 
            char2
        );
    }
    else // --- RETAIL / VOLUME PATH ---
    {
        unsigned int groupDigit = groupId % 10;
        unsigned int buildCode  = build % 1000;

        // Template: "Group-DigitSerial-Serial-AA-Build"
        // Example: "00330-01234-56789-AA-123"
        status = SafePrintf(
            outputBuffer, 
            48, 
            L"%05.5lu-%01.1lu%04.4lu-%05.5lu-%c%c%03.3lu", 
            groupSegment, 
            groupDigit, 
            serialCheck, 
            serialMid, 
            char1, 
            char2, 
            buildCode
        );
    }

    // 4. Error Handling
    if (status < 0) {
        LogError(status); // sub_18008A114
    }
    
    Cleanup(status); // sub_18008D904
    return status;
}

/**
 * Origin: Sppobjs.dll
 * Logic: Builds the massive 1272-byte DigitalProductId4 structure.
 */

// Original Name: sub_18013C100
NTSTATUS BuildDigitalProductId4(Context* ctx, void* unused, void* unused2, Pid4Buffer* output)
{
    NTSTATUS status;
    RPC_WSTR uuidString = NULL;
    wchar_t epidBuffer[64]; // Stores the Extended PID string
    
    // 1. Validation: The structure must be exactly 1272 bytes (0x4F8)
    if ( output->TotalSize != 1272 ) 
    {
        status = STATUS_INVALID_PARAMETER; // 0x80070057
        goto ExitWithError;
    }

    // 2. Activation Context: Get build-specific versioning info
    status = GetActivationContextInfo(&buildMinor, &buildMajor);
    if (status < 0) goto ExitWithError;

    // 3. String Generation: Format the Extended PID (EPID)
    // Uses the function we just looked at (sub_18013BFA8)
    status = FormatExtendedProductId(
        ctx, 
        ctx->MPC, 
        GetSystemDefaultLangID(), 
        buildMinor, 
        buildMajor, 
        epidBuffer);
    if (status < 0) goto ExitWithError;

    // 4. Memory Setup: Clear the buffer and set Version 4 header
    memset(output->PaddingArea, 0, 0x470);
    output->TotalSize = 1272;
    output->Version = 4;

    // 5. High-Speed Copy: Use XMM (128-bit) registers to move the EPID
    // This is the "v12 = v26[1]" logic in assembly
    Copy128Bit(output->EpidArea, epidBuffer);

    // 6. Hardware Binding (The "Lock")
    // Converts the binary Hardware UUID to a string
    status = UuidToStringW(ctx->HardwareId, &uuidString);
    if (status < 0) goto ExitWithError;

    // 7. Feature Flags: Mark if this is an Upgrade or Evaluation
    if (ctx->IsUpgrade > 1) 
        output->UpgradeFlag = 1;

    // 8. Security/Genuine State
    output->GenuineState = (ctx->Status != 0); // Is system genuine?

    // 9. Activation Seed: Copy specific licensing blobs
    output->ActivationBlob = ctx->LicenseBlob;

    // 10. The Integrity Seals (Hashing)
    // This function calls the CRC/Hashing engine multiple times:
    // A. Hash the Key ID
    sub_1800A4BB4(ctx->HashingContext, ctx->KeyId, 0x10);
    sub_18009BD7C(ctx->HashingContext, output->KeyIdHash);

    // B. Hash the ENTIRE 1272-byte structure (The Master Seal)
    sub_1800A4BB4(ctx->HashingContext, output, 1272);
    sub_18009BD7C(ctx->HashingContext, output->MasterHash);

ExitWithError:
    CleanupAndLog(status);
    return status;
}
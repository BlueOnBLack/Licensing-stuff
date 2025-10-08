// TSforge
// https://massgrave.dev/blog/tsforge

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
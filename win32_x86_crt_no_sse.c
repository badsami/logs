void __declspec(naked) _ftol()
{
	static const s32 offsets[2] = {0xBEFFFFFF, 0x3EFFFFFF};

	__asm
	{
		sub   esp,4
		fst   dword ptr [esp]
		mov   eax, dword ptr [esp]
		shr   eax, 29
		and   eax, 4
		fadd  dword ptr [offsets + eax]
		fistp dword ptr [esp]
		pop	 eax
		ret
	}
}

void _ftol2_sse()
{
	_ftol();
}

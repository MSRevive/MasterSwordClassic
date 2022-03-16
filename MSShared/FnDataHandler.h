#ifndef FN_DATA_HANDLER_H
#define FN_DATA_HANDLER_H

class CBasePlayer;

namespace FnDataHandler
{
	void Initialize(void);
	void Destroy(void);
	void Think(void);
	bool IsEnabled(void);

	void LoadCharacter(CBasePlayer* pPlayer);
	void LoadCharacter(CBasePlayer* pPlayer, int slot);
	void CreateOrUpdateCharacter(CBasePlayer* pPlayer, int slot, const char* data, int size, bool bIsUpdate);
	void DeleteCharacter(CBasePlayer* pPlayer, int slot);

	unsigned long long GetSteamID64(const char* id);
}

#endif // FN_DATA_HANDLER_H
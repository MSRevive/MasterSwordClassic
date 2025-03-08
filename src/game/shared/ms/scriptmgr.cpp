#include "inc_weapondefs.h"
#include "scriptmgr.h"
#include "logger.h"

CGameGroupFile ScriptMgr::m_GroupFile;
int ScriptMgr::m_TotalScripts = 0;

void ScriptMgr::RegisterScript(CScript *NewScript)
{
	if (!m_GroupFile.IsOpen())
		m_GroupFile.Open("pak0.pak");
	m_TotalScripts++;
}

void ScriptMgr::UnRegisterScript(CScript *NewScript)
{
	m_TotalScripts--;
	if( m_TotalScripts == 0 )	
		m_GroupFile.Close();	
}

void ScriptMgr::GameShutdown()
{
	//TODO: needed because scripts aren't freed on shutdown. If scripts are unloaded on shutdown, this is unnecessary - Solokiller
	m_GroupFile.Close();
}

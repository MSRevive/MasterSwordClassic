/*

	Script.h - Script file implementation

*/
#include <ctime>
class CEventList : public mslist<SCRIPT_EVENT *> //This class was created so I can store Events as pointers, but still access them as
{															//dereferenced objects
public:
	CEventList() : mslist<SCRIPT_EVENT *>() {}
	SCRIPT_EVENT &add( SCRIPT_EVENT &Event ) {
		SCRIPT_EVENT *pEvent = msnew(SCRIPT_EVENT);
		*pEvent = Event; mslist<SCRIPT_EVENT *>::add(pEvent);
		return *pEvent;
	}

	void erase(int idx)
	{
		SCRIPT_EVENT *pEvent = mslist<SCRIPT_EVENT *>::operator [] (idx);
		mslist<SCRIPT_EVENT *>::erase(idx);
		delete pEvent;
	}

	SCRIPT_EVENT &operator [] (const int idx) const {
		return *mslist<SCRIPT_EVENT *>::operator[](idx);
	}

	CEventList &operator = (const CEventList &OtherList)
	{
		clear();
		for(int i = 0; i < OtherList.size(); i++)
			add(OtherList[i]);

		return *this;
	}

	~CEventList() {
		clear();
	}

	void clear()
	{
		while(size())
			erase(0);
	}
};

class CScript : public IVariables
{
public:

	//If you ever add/change a member variable, make sure that
	//CopyAllData() will copy it correctly to new ents

	struct props
	{
		string_i ScriptFile;
		CBaseEntity	*pScriptedEnt;
		IScripted	*pScriptedInterface;
		CEventList Events;
		//Not copied
		SCRIPT_EVENT *CurrentEvent;
		bool PrecacheOnly;
		bool RemoveNextFrame;
		eventscope_e DefaultScope;
		bool AllowDupInclude;	//Allow duplicate include files
		bool Included;			//Whether this script file is included by another
		mslist<scriptsendcmd_t> PersistentSendCmds;	//Client event command that get transmitted to each joining player
		ulong	UniqueID;			//My unique ID.  Usually set for client-side scripts that get updated by server
		bool m_HandleRender;		//Handle certain callbacks
		ulong	m_Iteration;		//Current iteration, when called from calleventloop
	};
  props m;

	mslist<scriptvar_t>	m_Constants;	//Local constants
	static mslist<scriptvar_t> m_gVariables;	//Global variables
	static msscriptarrayhash GlobalScriptArrays; // MiB JUN2010_25
	static msscripthashhash mGlobalScriptHashes;
	static msscriptsethash mGlobalScriptSets;
	msstringlist m_Dependencies;	//Dependencies used by the current script.  Check that I can't #include a file twice
	static ulong m_gLastSendID;	//ID of last script sent to a client
	static ulong m_gLastLightID;	//ID of last dynamic light

#if !TURN_OFF_ALERT
	void conflict_check(msstring testvar, msstring testvar_type, msstring testvar_scope, int linenum); //Thothie JAN2013_09 variable conflict checks
#endif
	bool Spawn(string_i Filename, CBaseEntity *pScriptedEnt, IScripted *pScriptedInterface, bool PrecacheOnly = false, bool Casual = false);
	void RunScriptEvents( bool fOnlyRunNamedEvents = false );	//Runs all events
	void RunScriptEventByName( msstring_ref pszEventName, msstringlist *Parameters = NULL );	//Run one named event
	void CallLogged(msstring_ref title, std::clock_t start);
	bool ParseScriptFile( const char *pszScriptData );
	SCRIPT_EVENT *EventByName( const char *pszEventName );
	msstring_ref GetConst( msstring_ref pszText );
	scriptvar_t *FindVar( const char *pszName );
	msstring_ref GetVar( msstring_ref pszText );
	bool VarExists( msstring_ref pszText );
	scriptvar_t *SetVar( const char *pszVarName, const char *pszValue, bool fGlobal = false );
	scriptvar_t *SetVar( const char *pszVarName, const char *pszValue, SCRIPT_EVENT &Event );
	scriptvar_t *SetVar( const char *pszVarName, int iValue, bool fGlobal = false );
	scriptvar_t *SetVar( const char *pszVarName, float flValue, bool fGlobal = false );
	Vector StringToVec( msstring_ref String );
	void CopyAllData( CScript *pDestScript, CBaseEntity *pScriptedEnt, IScripted *pScriptedInterface );
	//int NewParseLine(std::string &pszCommandLine, int LineNum, SCRIPT_EVENT **pCurrentEvent, scriptcmd_list **pCurrentCmds, mslist<scriptcmd_list *> &ParentCmds);
	int ParseLine(const char *pszCommandLine /*in*/, int LineNum /*in*/, SCRIPT_EVENT **pCurrentEvent /*in/out*/, scriptcmd_list **pCurrentCmds /*in/out*/, mslist<scriptcmd_list *> &ParentCmds /*in/out*/);
	void SendScript( scriptsendcmd_t &SendCmd );	//Send script to client
	CBaseEntity *RetrieveEntity( msstring_ref Name );
  Vector DetermineOrigin(msstring & vsOrigin);
	void CallEventTimed(msstring_ref EventName, float Delay);

	//IScripted - Don't make CScript a part of IScripted or they allocate each other in a recursive loop
	//			  These functions are just imitating IScripted

	typedef scriptcmdbase_t<bool (CScript::*)(SCRIPT_EVENT &, scriptcmd_t &, msstringlist & )> scriptcmdscpp_cmdfunc_t;
	typedef std::map<msstring,scriptcmdscpp_cmdfunc_t> msfunchash_t;
	static msfunchash_t m_GlobalCmdHash; // MiB 30NOV_2014 Hashed commands for ScriptCmds.cpp

	void ErrorPrintCommand(const char * vsUniqueTag, SCRIPT_EVENT * vEvent, msstring & vsCmdName, msstringlist & vParams, int vParamStrt, char * vsText);

	static void Script_Setup( );
	static void ScriptGetterHash_Setup( ); // MiB 30NOV_2014 Function for adding functions to the Script.cpp hash
	static void CallScriptEventAll( msstring_ref EventName, msstringlist *Parameters );
	static void CallScriptPlayers( msstring_ref EventName, msstringlist *Parameters ); //Thothie - JUN2007a
	static void ClCallScriptPlayers( msstring_ref EventName, msstringlist *Parameters ); //Thothie - MAR2012_27
	static void ClXPlaySoundAll( msstring_ref sSample, const Vector &Origin, int sChannel, float sVolume, float sAttn, int sPitch ); //Thothie - MAR2012_28

	int Script_ParseLine( msstring_ref *pszCommandLine, scriptcmd_t &Cmd );
	bool Script_SetupEvent( SCRIPT_EVENT &Event, msstringlist *Parameters );
	bool Script_ExecuteEvent( SCRIPT_EVENT &Event, msstringlist *Parameters = NULL );
	bool Script_ExecuteCmds( SCRIPT_EVENT &Event, scriptcmd_list &Cmds );
	bool Script_ExecuteCmd( SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params );
	// Below are the functions for the ScriptCmds.cpp hash. Please keep them in alphabetical order here and where they are defined.
#define SCRIPTCMDSCPP_CMDS(a) bool ScriptCmd_##a( SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params )
	SCRIPTCMDSCPP_CMDS( ApplyEffect );
	SCRIPTCMDSCPP_CMDS( Array );
	SCRIPTCMDSCPP_CMDS( AttackProp );
	SCRIPTCMDSCPP_CMDS( bleed ); //Thothie DEC2014_13
	SCRIPTCMDSCPP_CMDS( breakloop ); //Thothie DEC2017_19 breakloop
	SCRIPTCMDSCPP_CMDS( resetloop ); //Thothie SEP2019_08 resetloop
	SCRIPTCMDSCPP_CMDS( CallClItemEvent );
	SCRIPTCMDSCPP_CMDS( CallEvent );
	SCRIPTCMDSCPP_CMDS( CapVar );
	SCRIPTCMDSCPP_CMDS( ChangeLevel );
	SCRIPTCMDSCPP_CMDS( ChatLog );
	SCRIPTCMDSCPP_CMDS( ClearPlayerHits );
	SCRIPTCMDSCPP_CMDS( ClEffect );
	SCRIPTCMDSCPP_CMDS( ClientCmd );
	SCRIPTCMDSCPP_CMDS( ClientEvent );
	SCRIPTCMDSCPP_CMDS( Companion );
	SCRIPTCMDSCPP_CMDS( ConflictCheck );
	SCRIPTCMDSCPP_CMDS( Create );
	SCRIPTCMDSCPP_CMDS( DarkenBloom );
	SCRIPTCMDSCPP_CMDS( Debug );
	SCRIPTCMDSCPP_CMDS( DeleteEntity );
	SCRIPTCMDSCPP_CMDS( DeleteEvent );
	SCRIPTCMDSCPP_CMDS( Desc );
	SCRIPTCMDSCPP_CMDS( DrainHP );
	SCRIPTCMDSCPP_CMDS( DrainStamina );
	SCRIPTCMDSCPP_CMDS( DropToFloor );
	SCRIPTCMDSCPP_CMDS( endgame ); //Thothie OCT2016_26 - exit command to deal with the fact we can't send the "quit" command anymore
	SCRIPTCMDSCPP_CMDS( exitevent );
	SCRIPTCMDSCPP_CMDS( Effect );
	SCRIPTCMDSCPP_CMDS( EmitSound );
	SCRIPTCMDSCPP_CMDS( EraseFile );
	SCRIPTCMDSCPP_CMDS( ErrorMessage );
	SCRIPTCMDSCPP_CMDS( GagPlayer );
	SCRIPTCMDSCPP_CMDS( GetEnts ); //MiB DEC2014_07 - "exitevent" command (exit.rtf)
	SCRIPTCMDSCPP_CMDS( GetItemArray );
	SCRIPTCMDSCPP_CMDS( GetPlayer );
	SCRIPTCMDSCPP_CMDS( GetPlayers );
	SCRIPTCMDSCPP_CMDS( GetPlayersNB );
	SCRIPTCMDSCPP_CMDS( GetPlayersArray );
	SCRIPTCMDSCPP_CMDS( GiveExp );
	SCRIPTCMDSCPP_CMDS( GiveHPMP );
	SCRIPTCMDSCPP_CMDS( Gravity );
  	SCRIPTCMDSCPP_CMDS( HashMap );
	SCRIPTCMDSCPP_CMDS( HelpTip );
	SCRIPTCMDSCPP_CMDS( HitMulti );
	SCRIPTCMDSCPP_CMDS( HudIcon );
	SCRIPTCMDSCPP_CMDS( If );
	SCRIPTCMDSCPP_CMDS( InfoMessage );
	SCRIPTCMDSCPP_CMDS( ItemRestrict );
	SCRIPTCMDSCPP_CMDS( Kill );
	SCRIPTCMDSCPP_CMDS( LocalPanel ); // MiB MAR2015_01 [LOCAL_PANEL] - Function for local panel options
  	SCRIPTCMDSCPP_CMDS( MarkDmg );
	SCRIPTCMDSCPP_CMDS( MathSet );
	SCRIPTCMDSCPP_CMDS( Message );
	SCRIPTCMDSCPP_CMDS( MessageAll );
	SCRIPTCMDSCPP_CMDS( moditem ); //Thothie OCT2016_18 Item Mods
	SCRIPTCMDSCPP_CMDS( MoveType );
	SCRIPTCMDSCPP_CMDS( Name );
	SCRIPTCMDSCPP_CMDS( NamePrefix );
	SCRIPTCMDSCPP_CMDS( NameUnique );
	SCRIPTCMDSCPP_CMDS( NpcMove );
	SCRIPTCMDSCPP_CMDS( Origin );
	SCRIPTCMDSCPP_CMDS( OverwriteSpell );
	SCRIPTCMDSCPP_CMDS( PlayerName );
	SCRIPTCMDSCPP_CMDS( PlayerTitle );
	SCRIPTCMDSCPP_CMDS( PlayMP3 );
	SCRIPTCMDSCPP_CMDS( PlaySound );
	SCRIPTCMDSCPP_CMDS( PrecacheFile );
	SCRIPTCMDSCPP_CMDS( ProjectileSize );
	SCRIPTCMDSCPP_CMDS( Quest );
	SCRIPTCMDSCPP_CMDS( RegisterDefaults );
	SCRIPTCMDSCPP_CMDS( RegisterEffect );
	SCRIPTCMDSCPP_CMDS( RegisterRace );
	SCRIPTCMDSCPP_CMDS( RegisterTexture );
	SCRIPTCMDSCPP_CMDS( RegisterTitle );
	SCRIPTCMDSCPP_CMDS( RemoveEffect );
	SCRIPTCMDSCPP_CMDS( RemoveScript );
	SCRIPTCMDSCPP_CMDS( RepeatDelay );
	SCRIPTCMDSCPP_CMDS( resetglobals ); //Thothie FEB2017_07 reset globals
	SCRIPTCMDSCPP_CMDS( Respawn );
	SCRIPTCMDSCPP_CMDS( Return );
	//Disabled since scripts shouldn't be handling character saving - Solokiller 5/10/2017
	//SCRIPTCMDSCPP_CMDS( SaveAllNow );
	//SCRIPTCMDSCPP_CMDS( SaveNow );
	SCRIPTCMDSCPP_CMDS( ServerCmd );
  	SCRIPTCMDSCPP_CMDS( Set );
	SCRIPTCMDSCPP_CMDS( SetAlive );
	SCRIPTCMDSCPP_CMDS( SetAngle );
	SCRIPTCMDSCPP_CMDS( SetAtkSpeed );
	SCRIPTCMDSCPP_CMDS( SetBBox );
	SCRIPTCMDSCPP_CMDS( SetCallBack );
	SCRIPTCMDSCPP_CMDS( SetCVar );
	SCRIPTCMDSCPP_CMDS( SetEnv );
	SCRIPTCMDSCPP_CMDS( SetExpStat );
	SCRIPTCMDSCPP_CMDS( SetFollow );
	SCRIPTCMDSCPP_CMDS( SetGaitSpeed );
	SCRIPTCMDSCPP_CMDS( SetModel );
	SCRIPTCMDSCPP_CMDS( SetModelBody );
	SCRIPTCMDSCPP_CMDS( SetProp );
	SCRIPTCMDSCPP_CMDS( SetPVP );
	SCRIPTCMDSCPP_CMDS( SetQuality );
	SCRIPTCMDSCPP_CMDS( setquantity ); //Thothie MAR2015_15
	SCRIPTCMDSCPP_CMDS( SetSolid );
	SCRIPTCMDSCPP_CMDS( SetTrans );
	SCRIPTCMDSCPP_CMDS( SetVar );
	SCRIPTCMDSCPP_CMDS( SetViewModelProp );
	SCRIPTCMDSCPP_CMDS( SetVolume );
	SCRIPTCMDSCPP_CMDS( SetWearPos );
	SCRIPTCMDSCPP_CMDS( ScriptFlags );
	SCRIPTCMDSCPP_CMDS( SoundPlay3D );
	SCRIPTCMDSCPP_CMDS( SoundPMPlay );
	SCRIPTCMDSCPP_CMDS( SoundSetVolume );
	SCRIPTCMDSCPP_CMDS( StoreEntity );
	SCRIPTCMDSCPP_CMDS( StrAdd );
	SCRIPTCMDSCPP_CMDS( StrConc );
	SCRIPTCMDSCPP_CMDS( SPlayViewAnim );
	SCRIPTCMDSCPP_CMDS( syncitem ); //Thothie OCT2016_04 - sync item data from server to client
	SCRIPTCMDSCPP_CMDS( teleportdest ); //Thothie OCT2015_18 - teleport to specific info_teleport_destination
	SCRIPTCMDSCPP_CMDS( TokenAdd );
	SCRIPTCMDSCPP_CMDS( TokenDel );
	SCRIPTCMDSCPP_CMDS( TokenScramble );
	SCRIPTCMDSCPP_CMDS( TokenSet );
	SCRIPTCMDSCPP_CMDS( ToRandomSpawn );
	SCRIPTCMDSCPP_CMDS( ToSpawn );
	SCRIPTCMDSCPP_CMDS( UseTrigger );
	SCRIPTCMDSCPP_CMDS( VectorAdd );
	SCRIPTCMDSCPP_CMDS( VectorMultiply );
	SCRIPTCMDSCPP_CMDS( VectorSet );
	SCRIPTCMDSCPP_CMDS( Velocity );
	SCRIPTCMDSCPP_CMDS( Volume );
	SCRIPTCMDSCPP_CMDS( Weight );
	SCRIPTCMDSCPP_CMDS( WipeSpell );
	SCRIPTCMDSCPP_CMDS( WriteLine );
	SCRIPTCMDSCPP_CMDS( XDoDamage );
	SCRIPTCMDSCPP_CMDS( DebugEntities ); //MiB MAR2019_28 Internal Script Debug
	SCRIPTCMDSCPP_CMDS( SetEntForceSend );
	SCRIPTCMDSCPP_CMDS( SetEntNoSend );
#undef SCRIPTCMDSCPP_CMDS

	typedef scriptcmdbase_t<msstring (CScript::*)(msstring&, msstring&, msstringlist&)> scriptcpp_cmdfunc_t;
    typedef std::map<msstring,scriptcpp_cmdfunc_t> msgetterhash_t;
    static msgetterhash_t m_GlobalGetterHash; // MiB 30NOV_2014 Hashed commands for Script.cpp
	// Below are the functions for the Script.cpp hash. Please keep them in alphabetical order here and where they are defined.
#define SCRIPTCPP_GETTER(a) msstring ScriptGetter_##a( msstring& FullName, msstring& ParserName, msstringlist& Params )
	SCRIPTCPP_GETTER( AlphaNum );
	SCRIPTCPP_GETTER( AngleDiff );
	SCRIPTCPP_GETTER( Angles );
	SCRIPTCPP_GETTER( Angles3d );
	SCRIPTCPP_GETTER( AnimExists );
	SCRIPTCPP_GETTER( CanDamage );
	SCRIPTCPP_GETTER( CapFirst );
	SCRIPTCPP_GETTER( clcol ); //Thothie DEC2014_10 - $clcol - somewhat better client<->server color matching
	SCRIPTCPP_GETTER( Cone );
	SCRIPTCPP_GETTER( ConstGame );
	SCRIPTCPP_GETTER( ConstLocalPlayer );
	SCRIPTCPP_GETTER( ConstMoveType );
	SCRIPTCPP_GETTER( ConstSnd );
	SCRIPTCPP_GETTER( Dir );
	SCRIPTCPP_GETTER( Dist );
	SCRIPTCPP_GETTER( Eval );
	SCRIPTCPP_GETTER( FileSize );
	SCRIPTCPP_GETTER( Float );
	SCRIPTCPP_GETTER( Func );
	SCRIPTCPP_GETTER( Get );
	SCRIPTCPP_GETTER( GetAttackProp );
	SCRIPTCPP_GETTER( GetArray );
	SCRIPTCPP_GETTER( GetByName );
	SCRIPTCPP_GETTER( GetCl );
	SCRIPTCPP_GETTER( getcl_beam ); //Thothie DEC2014_10 - beam_update
	SCRIPTCPP_GETTER( GetClTSphere );
	SCRIPTCPP_GETTER( GetConst );
	SCRIPTCPP_GETTER( GetContents );
	SCRIPTCPP_GETTER( GetCVar );
	SCRIPTCPP_GETTER( FileHash ); //Wishbone MAR2016 - get a file's hash.
	SCRIPTCPP_GETTER( GetFileLine );
	SCRIPTCPP_GETTER( GetFindToken );
	SCRIPTCPP_GETTER( GetGroundHeight );
	SCRIPTCPP_GETTER( GetHashMap );
	SCRIPTCPP_GETTER( GetInSphere );
	SCRIPTCPP_GETTER( GetJoinType );
	SCRIPTCPP_GETTER( GetLastMap );
	SCRIPTCPP_GETTER( GetMapLegit );
	SCRIPTCPP_GETTER( GetQuestData );
	SCRIPTCPP_GETTER( GetScriptFlag );
	SCRIPTCPP_GETTER( GetSet );
	SCRIPTCPP_GETTER( GetSkillName );
	SCRIPTCPP_GETTER( GetSkillRatio );
	SCRIPTCPP_GETTER( GetSkyHeight );
	SCRIPTCPP_GETTER( GetTakeDmg );
	SCRIPTCPP_GETTER( GetTime );
	SCRIPTCPP_GETTER( GetToken );
	SCRIPTCPP_GETTER( GetTokenAmt );
	SCRIPTCPP_GETTER( GetTraceLine );
	SCRIPTCPP_GETTER( GetTSphereAndBox );
	SCRIPTCPP_GETTER( GetUnderSky );
	SCRIPTCPP_GETTER( Int );
	SCRIPTCPP_GETTER( indiam ); //Thothie APR2016_15 $indiam/$indiam2D
	SCRIPTCPP_GETTER( inrange ); //Thothie APR2016_15 $inrange
	SCRIPTCPP_GETTER( ItemExists );
	SCRIPTCPP_GETTER( LCase );
	SCRIPTCPP_GETTER( Len );
	SCRIPTCPP_GETTER( MathReturn );
	SCRIPTCPP_GETTER( MapExists );
	SCRIPTCPP_GETTER( MinMax );
	SCRIPTCPP_GETTER( Mid );
	SCRIPTCPP_GETTER( Neg );
	SCRIPTCPP_GETTER( Num );
	SCRIPTCPP_GETTER( Pass ); //Thothie DEC2017_05
	SCRIPTCPP_GETTER( Quote );
	SCRIPTCPP_GETTER( Rand );
	SCRIPTCPP_GETTER( random_of_set ); //Thothie MAY2016_04
	SCRIPTCPP_GETTER( get_random_token ); //Thothie MAY2016_04
	SCRIPTCPP_GETTER( RelPos );
	SCRIPTCPP_GETTER( RelVel );
	SCRIPTCPP_GETTER( ReplaceOrInsert );
	SCRIPTCPP_GETTER( ScanShape );
	SCRIPTCPP_GETTER( SearchString );
  	SCRIPTCPP_GETTER( ShapeCylinder );
  	SCRIPTCPP_GETTER( ShapeRect );
	SCRIPTCPP_GETTER( ShapeSphere );
	SCRIPTCPP_GETTER( SortEntList );
	SCRIPTCPP_GETTER( StrAdd );
	SCRIPTCPP_GETTER( StringRightOrLeft );
	SCRIPTCPP_GETTER( StringUpToOrFrom );
	SCRIPTCPP_GETTER( Subst ); //Thothie DEC2017_05
	SCRIPTCPP_GETTER( TimeStamp );
	SCRIPTCPP_GETTER( UCase );
	SCRIPTCPP_GETTER( Vec );
	SCRIPTCPP_GETTER( VecLen );
	SCRIPTCPP_GETTER( tokenize ); //Thothie SEP2019_15
	SCRIPTCPP_GETTER( WithinBox );
	SCRIPTCPP_GETTER( GetItemTable ); //MiB FEB2019_23 - item table getter
	SCRIPTCPP_GETTER( Conjunction ); //MiB FEB2019_23 - Extended If Conditionals
#undef SCRIPTCPP_GETTER

	//bool ScriptCmd_Hud( SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params );
	//bool ScriptCmd_NpcMove( SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params );
	msstring_ref GetScriptVar( msstring_ref EventName );

	void ScriptedEffect( msstringlist &Params );
	void CLScriptedEffect( msstringlist &Params );
#ifndef VALVE_DLL
	msstring_ref CLGetCurrentTempEntProp( msstring &Prop );
	msstring_ref CLGetEntProp( struct cl_entity_s *pclEntity, msstringlist &Params );
	msstring_ref CLGetBeamProp( int beamid, msstringlist &Params ); //DEC2014_09 Thothie - beam_update
#endif

	CScript( );
	~CScript( );
};

#undef SCRIPTVAR
#define SCRIPTVAR GetFirstScriptVar
bool GetString(char *Return, size_t size, const char *sentence, int start, const char *endchars);
::mslist<std::string> GetParams(std::string const &str);
void ReplaceChar(char *pString, char org, char dest);
float GetNumeric(const char *pszText);

enum scriptconatiner_e {
	MS_SCRIPT_UKNOWN,	//GenericItemPrecache hasn't been called yet, so we dont know
	MS_SCRIPT_LIBRARY,	//Scripts loaded from pak0.pak
	MS_SCRIPT_DIR		//Scripts loaded from /scripts
};

struct globalscriptinfo_t
{
	scriptconatiner_e Container;
	const char *ContainerName;
};

extern globalscriptinfo_t *g_MSScriptInfo;
#define FILE_DEV_ITEMLIST "scripts/items.txt"
#define FILE_ITEMLIST "items.txt"

#ifdef VALVE_DLL
	#define SCRIPT_ID_START 0		//Server: ID of last script sent to a client
#else
	#define SCRIPT_ID_START 10000	//Client: ID of next script to be created
#endif

//MiB Feb2008a - for the scriptio system File I/O
//- moved somewhere else?
/*struct scriptfile_t
{
private:
		int curline;
public:
	msstring fileName;
	bool endoffile;
	bool nofile;

	msstringlist Lines;

	void JumpToLine( int line ) { curline = line; }

	scriptfile_t &operator = ( const msstring_ref a );

	void Open( msstring_ref a );
	void DeleteFile();
	void Reset();

	msstring ScriptFile_ReadLine();
	msstring ScriptFile_ReadLine( int line );
	//void ChatLog( msstring_ref a, msstring action, msstring line ); //Thothie Phayle
	void ScriptFile_WriteLine( msstring line );
	void ScriptFile_WriteLine( msstring line, int lineNum, bool overwrite = false);
														 //Only used if you're "inserting" a line
														 //Choice between inserting or overwriting
};
*/

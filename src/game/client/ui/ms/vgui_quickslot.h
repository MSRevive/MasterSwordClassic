//
//  This should only be included by vgui_HUD.cpp
//
#pragma once

#include "syntax/syntax.h"
#include "vgui_ihud.h"
#include <limits>

static COLOR Color_Text_QuickSlot_Weapon(255, 255, 255, 0);
static COLOR Color_Text_QuickSlot_Spell(120, 255, 255, 0);
static COLOR Color_Text_QuickSlot_Arrow(0, 255, 128, 0);

#define QUICKSLOT_PREF_TIMEOUT m_CVARTimeout->value
#define QUICKSLOT_WEAPON "weapon"
#define QUICKSLOT_SPELL "spell"
#define QUICKSLOT_ARROW "arrow"
#define QUICKSLOT_USE "use"

class VGUI_QuickSlot : public Panel, public IHUD
{
public:
	MSLabel *m_Name;
	//msstring			m_Type;
	uint m_FirstQuickItem; //When I return to this, go blank
	//uint				m_SelectedSlot.ID;	//
	bool m_Active;
	int m_RepeatType;
	float m_TimeLastSelect;
	cvar_t *m_CVARTimeout;
	quickslot_t m_SelectedSlot;
	float m_StartedHolding;
	int m_HoldSlot;

	typedef enum
	{
		PROJ_ANY,
		PROJ_ARROW,
		PROJ_BOLT
	} projtype_e;
	projtype_e ProjType;		 //Slightly hacky way of checking what arrow types to be shown when scrolling
	CGenericItem *pGenericArrow; //Generic arrow place-holder
	CGenericItem *pGenericBolt;	 //Generic bolt place-holder
								 //These aren't actually ON the player, but I want them to be selectable.
	int m_Cycle;				 //Generic Arrow/Bolt are first - Keep track of Cycle.

#define QUICKSLOT_H YRES(14)

	VGUI_QuickSlot(Panel *pParent) : Panel(XRES(170), ScreenHeight - QUICKSLOT_H, ScreenWidth, QUICKSLOT_H)
	{
		setParent(pParent);
		SetBGColorRGB(Color_Transparent);

		m_Name = new MSLabel(this, "", 0, 0);
		m_Name->setFont(g_FontID);
		m_Name->SetFGColorRGB(Color_Text_White);

		SelectItem(NULL);
		m_TimeLastSelect = 0;

		m_CVARTimeout = gEngfuncs.pfnGetCvarPointer("ms_quickslot_timeout");

		pGenericArrow = CGenericItemMgr::GetGlobalGenericItemByName("proj_arrow_generic");
		pGenericArrow->m_DisplayName = "Crude Wooden Arrow";
		pGenericBolt = CGenericItemMgr::GetGlobalGenericItemByName("proj_bolt_generic");
		pGenericBolt->m_DisplayName = "Crude Wooden Bolt";
	}

	void Update()
	{
		//Check if I've been holding a slot button for 2 seconds
		if (m_StartedHolding && gpGlobals->time - m_StartedHolding > 2)
		{
			m_StartedHolding = 0;
			AssignSlot(m_HoldSlot);
		}

		if (!m_Active)
			return;

		if (gpGlobals->time - m_TimeLastSelect >= QUICKSLOT_PREF_TIMEOUT)
		{
			//ConfirmItem( );
			SelectItem(NULL);
		}
	}

	void Select(msstring &Type)
	{
		quickslottype_e NewType = (quickslottype_e)0;
		if (Type == QUICKSLOT_WEAPON)
			NewType = QS_ITEM;
		else if (Type == QUICKSLOT_SPELL)
			NewType = QS_SPELL;
		else if (Type == QUICKSLOT_ARROW)
		{
			ClientCmd("choosingarrow\n"); //NOV2014_16 - tell server I'm busy pickin arrows
			NewType = QS_ARROW;
		}
		else if (Type == QUICKSLOT_USE)
			NewType = (quickslottype_e)0;
		else
			return;

		if (NewType != m_SelectedSlot.Type && m_Active) //Changed select types while active... reset
			SelectItem(NULL);

		m_SelectedSlot.Type = NewType;
		m_TimeLastSelect = gpGlobals->time;

		if (m_SelectedSlot.Type == QS_ITEM)
		{
			CGenericItem *pItem = player.GetItemInInventory(m_SelectedSlot.ID, true, false, true);
			if (pItem)
			{
				if (pItem->m_iId != m_FirstQuickItem)
				{
					SelectItem(pItem, m_SelectedSlot.Type);
					m_SelectedSlot.ID = pItem->m_iId;
					if (m_FirstQuickItem == std::numeric_limits<uint>::max())
						m_FirstQuickItem = pItem->m_iId;
					if (MSCLGlobals::DefaultHUDSounds.QuickSlot_Select)
						PlayHUDSound("ui/buttonclick.wav", 1.0f); //Thothie FEB2008a - original: PlayHUDSound( MSCLGlobals::DefaultHUDSounds.QuickSlot_Select, 1.0f );
				}
				else
					//Got back to first item.  Hide panel and reset
					SelectItem(NULL);
			}
		}
		else if (m_SelectedSlot.Type == QS_SPELL && player.m_SpellList.size())
		{
			quickslot_t QuickSlot;
			QuickSlot.Active = true;
			QuickSlot.Type = QS_SPELL;

			//If prev spell is within range
			if (m_SelectedSlot.ID >= 0 && m_SelectedSlot.ID < player.m_SpellList.size())
			{
				//If prev spell is not the last spell in list
				if (m_SelectedSlot.ID < player.m_SpellList.size() - 1)
				{
					//Choose the next spell in the list
					m_SelectedSlot.ID++;
					QuickSlot.ID = m_SelectedSlot.ID;
				}
				else if (m_SelectedSlot.ID == player.m_SpellList.size() - 1)
					QuickSlot.Active = false; //Last item was last item - Hide panel and reset
			}

			//Prev spell is out of range... start with first spell
			else if (player.m_SpellList.size()) //No last spell selected.  Start with the first spell
			{
				QuickSlot.ID = 0;
				m_SelectedSlot.ID = 0;
			}

			if (QuickSlot.Active)
			{
				SelectSlot(QuickSlot);
				if (MSCLGlobals::DefaultHUDSounds.QuickSlot_Select)
					PlayHUDSound("ui/buttonclick.wav", 1.0f); //Thothie FEB2008a - original: PlayHUDSound( MSCLGlobals::DefaultHUDSounds.QuickSlot_Select, 1.0f );
			}
			else
				SelectItem(NULL);
		}
		else if (m_SelectedSlot.Type == QS_ARROW)
		{
			CGenericItem *pArrow;
			ClientCmd("choosingarrow\n"); //NOV2014_16 - tell server I'm busy pickin arrows

			bool GENERIC = true;
			if (!m_Cycle)
			{
				CGenericItem *pCur = player.ActiveItem();
				if (pCur->m_Attacks.size())
				{
					if (msstring(pCur->m_Attacks[0].sProjectileType).contains("arrow"))
						ProjType = PROJ_ARROW;
					else if (msstring(pCur->m_Attacks[0].sProjectileType).contains("bolt"))
						ProjType = PROJ_BOLT;
					else
						ProjType = PROJ_ANY;
				}

				if (ProjType == PROJ_BOLT)
					pArrow = pGenericBolt;
				else
					pArrow = pGenericArrow;
			}
			else if (m_Cycle == 1 && ProjType == PROJ_ANY)
				pArrow = pGenericBolt;
			else
			{
				GENERIC = false;
				msstring ArrowName = "proj_";
				if (ProjType == PROJ_ARROW)
					ArrowName += "arrow_";
				else if (ProjType == PROJ_BOLT)
					ArrowName += "bolt_";

				pArrow = player.GetItemInInventory(m_SelectedSlot.ID, false, false, true, ArrowName);
			}

			++m_Cycle;

			if (pArrow)
			{
				if (pArrow->m_iId != m_FirstQuickItem)
				{
					SelectItem(pArrow, m_SelectedSlot.Type);
					/*if ( !GENERIC )*/ m_SelectedSlot.ID = pArrow->m_iId;
					if (m_FirstQuickItem == std::numeric_limits<uint>::max() && !GENERIC)
						m_FirstQuickItem = pArrow->m_iId;
					if (MSCLGlobals::DefaultHUDSounds.QuickSlot_Select)
					{
						PlayHUDSound("ui/buttonclick.wav", 1.0f);
					}
				}
				else
				{
					SelectItem(NULL);
				}
			}
			else
			{
				SelectItem(NULL);
			}
		}
	}

	void SelectItem(CGenericItem *pItem, quickslottype_e Type = QS_ITEM)
	{
		if (pItem)
		{
			msstring ItemName = SPEECH::ItemName(pItem, true);
			msstring Text = msstring("") + ((Type == QS_SPELL) ? "Cast " : "") + ItemName;
			if (Type == QS_ARROW)
			{
				if (pItem == pGenericArrow || pItem == pGenericBolt)
					Text += " (Infinite)";
			}

			m_Name->setText(Text);

			//Must set it to a variable, because SetFGColorRGB is a macro
			COLOR Color = (Type == QS_SPELL ? Color_Text_QuickSlot_Spell : Color_Text_QuickSlot_Weapon);
			Color = Type == QS_ARROW ? Color_Text_QuickSlot_Arrow : Color;
			m_Name->SetFGColorRGB(Color);
			m_Active = true;
			m_SelectedSlot.Active = true;
			m_Name->setVisible(true);
		}
		else
		{
			m_Active = false;
			m_SelectedSlot.Active = false;
			m_Name->setVisible(false);
			m_FirstQuickItem = std::numeric_limits<uint>::max();
			m_SelectedSlot.ID = std::numeric_limits<uint>::max();
			m_Cycle = 0;
		}
	}

	bool SelectSlot(quickslot_t &QuickSlot)
	{
		if (QuickSlot.Type == QS_ITEM)
		{
			CGenericItem *pItem = player.FindItem(QuickSlot.ID);
			if (pItem)
			{
				SelectItem(pItem, QuickSlot.Type);
				return true;
			}
		}
		else if (QuickSlot.Type == QS_SPELL)
		{
			// MiB MAR2019_13 - Clean up of "temporary items" in favor of global table
			CGenericItem* pItem = CGenericItemMgr::GetGlobalGenericItemByName(player.m_SpellList[QuickSlot.ID], true);
			if (pItem)
			{
				SelectItem(pItem, QuickSlot.Type);
				return true;
			}
		}

		return false;
	}

	bool ConfirmItem()
	{
		return ConfirmItem(m_SelectedSlot);
	}

	bool ConfirmItem(quickslot_t &QuickSlot)
	{
		if (!QuickSlot.Active)
			return false;

		char cTemp[64] = "";
		if (QuickSlot.Type == QS_ITEM)
		{
			 _snprintf(cTemp, sizeof(cTemp),  "inv transfer %u 0\n",  QuickSlot.ID );
			ClientCmd(cTemp);
		}
		else if (QuickSlot.Type == QS_SPELL)
		{
			if (QuickSlot.ID >= 0 && QuickSlot.ID < player.m_SpellList.size()) //Validate here, just in case
			{
				 _snprintf(cTemp, sizeof(cTemp),  "prep %s\n",  player.m_SpellList[QuickSlot.ID].c_str() );
				ClientCmd(cTemp);
			}
		}
		else if (QuickSlot.Type == QS_ARROW)
		{
			if (m_Cycle == 1 ||
				(m_Cycle == 2 && ProjType == PROJ_ANY))
			{
				msstring GEN = m_Cycle == 1 ? (ProjType == PROJ_BOLT ? "BOLT" : "ARROW") : "BOLT";
				 _snprintf(cTemp, sizeof(cTemp),  "selectarrow GENERIC_%s",  GEN.c_str() );
				player.m_ChosenArrow = NewGenericItem(QuickSlot.ID == pGenericArrow->m_iId ? "proj_arrow_generic" : "proj_bolt_generic");
			}
			else
			{
				 _snprintf(cTemp, sizeof(cTemp),  "selectarrow %u",  QuickSlot.ID );
				player.m_ChosenArrow = MSUtil_GetItemByID(QuickSlot.ID);
			}

			ClientCmd(cTemp);
		}

		if (MSCLGlobals::DefaultHUDSounds.QuickSlot_Confirm)
			PlayHUDSound(MSCLGlobals::DefaultHUDSounds.QuickSlot_Confirm, 1.0f);

		SelectItem(NULL);
		return true;
	}

	void SlotPressed(bool fDown, int Slot)
	{
		if (fDown)
		{
			//Slot button pressed
			m_StartedHolding = gpGlobals->time;
			m_HoldSlot = Slot;
		}
		else
		{
			//Slot button released
			if (!m_StartedHolding || gpGlobals->time - m_StartedHolding >= 2)
				return;

			m_StartedHolding = 0;

			//Let go of slot before 2 seconds.  Use this slot
			quickslot_t &QuickSlot = player.m_QuickSlots[m_HoldSlot];
			if (!QuickSlot.Active)
				return;

			ConfirmItem(QuickSlot);
		}
	}

	void AssignSlot(int Slot)
	{
		if (Slot < 0 || Slot >= MAX_QUICKSLOTS)
			return;

		msstring Cmd = "quickslot create ";
		Cmd += Slot;
		Cmd += " current\n";
		ServerCmd(Cmd);

		if (MSCLGlobals::DefaultHUDSounds.QuickSlot_Select)
			PlayHUDSound(MSCLGlobals::DefaultHUDSounds.QuickSlot_Select, 1.0f);
	}
};
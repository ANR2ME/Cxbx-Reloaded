// ******************************************************************
// *
// *    .,-:::::    .,::      .::::::::.    .,::      .:
// *  ,;;;'````'    `;;;,  .,;;  ;;;'';;'   `;;;,  .,;; 
// *  [[[             '[[,,[['   [[[__[[\.    '[[,,[['  
// *  $$$              Y$$$P     $$""""Y$$     Y$$$P    
// *  `88bo,__,o,    oP"``"Yo,  _88o,,od8P   oP"``"Yo,  
// *    "YUMMMMMP",m"       "Mm,""YUMMMP" ,m"       "Mm,
// *
// *   Cxbx->Win32->CxbxKrnl->EmuShared.cpp
// *
// *  This file is part of the Cxbx project.
// *
// *  Cxbx and Cxbe are free software; you can redistribute them
// *  and/or modify them under the terms of the GNU General Public
// *  License as published by the Free Software Foundation; either
// *  version 2 of the license, or (at your option) any later version.
// *
// *  This program is distributed in the hope that it will be useful,
// *  but WITHOUT ANY WARRANTY; without even the implied warranty of
// *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// *  GNU General Public License for more details.
// *
// *  You should have recieved a copy of the GNU General Public License
// *  along with this program; see the file COPYING.
// *  If not, write to the Free Software Foundation, Inc.,
// *  59 Temple Place - Suite 330, Bostom, MA 02111-1307, USA.
// *
// *  (c) 2002-2003 Aaron Robinson <caustik@caustik.com>
// *
// *  All rights reserved
// *
// ******************************************************************
#define _CXBXKRNL_INTERNAL
#define _XBOXKRNL_LOCAL_

#include "EmuShared.h"

#undef FIELD_OFFSET     // prevent macro redefinition warnings
#include <windows.h>
#include <stdio.h>

// ******************************************************************
// * shared memory
// ******************************************************************
CXBXKRNL_API EmuShared *g_EmuShared = NULL;

// ******************************************************************
// * func: EmuSharedInit
// ******************************************************************
CXBXKRNL_API void EmuShared::Init()
{
    static HANDLE hMapObject = NULL;
    
    bool init = true;

    // ******************************************************************
    // * Prevent multiple initializations
    // ******************************************************************
    if(hMapObject != NULL)
        return;

    // ******************************************************************
    // * Create the shared memory "file"
    // ******************************************************************
    {
        hMapObject = CreateFileMapping
        ( 
            INVALID_HANDLE_VALUE,   // Paging file
            NULL,                   // default security attributes
            PAGE_READWRITE,         // read/write access
            0,                      // size: high 32 bits
            sizeof(EmuShared),      // size: low 32 bits
            "Local\\EmuShared"      // name of map object
        );

        if(hMapObject == NULL)
            return; // TODO: Fail gracefully

        if(GetLastError() == ERROR_ALREADY_EXISTS)
            init = false;
    }

    // ******************************************************************
    // * Memory map this file
    // ******************************************************************
    {
        g_EmuShared = (EmuShared*)MapViewOfFile
        (
            hMapObject,     // object to map view of
            FILE_MAP_WRITE, // read/write access
            0,              // high offset:  map from
            0,              // low offset:   beginning
            0               // default: map entire file
        );

        if(g_EmuShared == NULL) 
            return; // TODO: Fail gracefully
    }

    // ******************************************************************
    // * Executed only on first initialization of shared memory
    // ******************************************************************
    if(init)
    {
        g_EmuShared->LoadInputConfiguration();
        g_EmuShared->InitInputConfiguration();
    }
}

// ******************************************************************
// * func: EmuSharedCleanup
// ******************************************************************
CXBXKRNL_API void EmuShared::Cleanup()
{
    UnmapViewOfFile(g_EmuShared);
}

// ******************************************************************
// * func: SetInputConfiguration
// ******************************************************************
CXBXKRNL_API void EmuShared::SetInputConfiguration(InputConfig *x_InputConfig)
{
    Lock();

    memcpy(&m_InputConfig, x_InputConfig, sizeof(InputConfig));

    m_dwChangeID++;

    Unlock();

    return;
}

// ******************************************************************
// * func: GetInputConfiguration
// ******************************************************************
CXBXKRNL_API void EmuShared::GetInputConfiguration(InputConfig *x_InputConfig)
{
	Lock();

	memcpy(x_InputConfig, &m_InputConfig, sizeof(InputConfig));

	Unlock();

    return;
}

// ******************************************************************
// * func: LoadInputConfiguration
// ******************************************************************
CXBXKRNL_API void EmuShared::LoadInputConfiguration()
{
    Lock();

    // ******************************************************************
    // * Load configuration from registry
    // ******************************************************************
    {
        /*
        DWORD   dwDisposition, dwType, dwSize;
        HKEY    hKey;

        if(RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Cxbx\\Input", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_QUERY_VALUE, NULL, &hKey, &dwDisposition) == ERROR_SUCCESS)
        {
            for(int v=0;v<MAX_INPUT_DEVICES;v++)
            {
            }

            RegCloseKey(hKey);
            /*
            dwType = REG_DWORD; dwSize = sizeof(DWORD);
            RegQueryValueEx(hKey, "CxbxDebug", NULL, &dwType, (PBYTE)&m_CxbxDebug, &dwSize);

            dwType = REG_DWORD; dwSize = sizeof(DWORD);
            RegQueryValueEx(hKey, "KrnlDebug", NULL, &dwType, (PBYTE)&m_KrnlDebug, &dwSize);

            dwType = REG_DWORD; dwSize = sizeof(DWORD);
            RegQueryValueEx(hKey, "RecentXbe", NULL, &dwType, (PBYTE)&m_dwRecentXbe, &dwSize);

            dwType = REG_DWORD; dwSize = sizeof(DWORD);
            RegQueryValueEx(hKey, "RecentExe", NULL, &dwType, (PBYTE)&m_dwRecentExe, &dwSize);

            dwType = REG_DWORD; dwSize = sizeof(DWORD);
            RegQueryValueEx(hKey, "AutoConvertToExe", NULL, &dwType, (PBYTE)&m_bAutoConvertToExe, &dwSize);

            dwType = REG_SZ; dwSize = 260;
            RegQueryValueEx(hKey, "CxbxDebugFilename", NULL, &dwType, (PBYTE)m_CxbxDebugFilename, &dwSize);

            dwType = REG_SZ; dwSize = 260;
            RegQueryValueEx(hKey, "KrnlDebugFilename", NULL, &dwType, (PBYTE)m_KrnlDebugFilename, &dwSize);

            int v=0;

            for(v=0;v<m_dwRecentXbe;v++)
            {
                char buffer[32];

                sprintf(buffer, "RecentXbe%d", v);

                m_szRecentXbe[v] = (char*)calloc(1, 260);

                dwType = REG_SZ; dwSize = 260;
                RegQueryValueEx(hKey, buffer, NULL, &dwType, (PBYTE)m_szRecentXbe[v], &dwSize);
            }

            for(v=0;v<m_dwRecentExe;v++)
            {
                char buffer[32];

                sprintf(buffer, "RecentExe%d", v);

                m_szRecentExe[v] = (char*)calloc(1, 260);

                dwType = REG_SZ; dwSize = 260;
                RegQueryValueEx(hKey, buffer, NULL, &dwType, (PBYTE)m_szRecentExe[v], &dwSize);
            }
        }
        */
    }

    Unlock();

    return;
}

// ******************************************************************
// * func: SaveInputConfiguration
// ******************************************************************
CXBXKRNL_API void EmuShared::SaveInputConfiguration()
{
    Lock();

    // ******************************************************************
    // * Save configuration to registry
    // ******************************************************************
    {
        DWORD dwDisposition, dwType, dwSize;
        HKEY  hKey;

        if(RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Cxbx\\Input", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, &dwDisposition) == ERROR_SUCCESS)
        {
            char szBuffer[128];

            for(int d=0;d<MAX_INPUT_DEVICES;d++)
            {
                const char *szCur = m_InputConfig.GetDeviceName(d);

                if(szCur[0] == '\0')
                    continue;

                sprintf(szBuffer, "Device Name #%d", d);

                dwType = REG_SZ; dwSize = 260;
                RegSetValueEx(hKey, szBuffer, 0, dwType, (PBYTE)szCur, dwSize);
            }

            for(int c=0;c<INPUT_DEVICE_COMPONENT_COUNT;c++)
            {
                int RawData[3] = {-1, -1, 0};

                m_InputConfig.Get((InputDeviceComponent)c, &RawData[0], &RawData[1], &RawData[2]);

                sprintf(szBuffer, "Component : %s", g_InputDeviceTitle[c]);

                dwType = REG_BINARY; dwSize = sizeof(RawData);
                RegSetValueEx(hKey, szBuffer, 0, dwType, (PBYTE)&RawData, dwSize);
            }
        }
    }

    Unlock();

    return;
}
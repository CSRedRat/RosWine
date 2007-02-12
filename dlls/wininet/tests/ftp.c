/*
 * Wininet - ftp tests
 *
 * Copyright 2007 Paul Vriens
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/*
 * FIXME:
 *     Use InternetGetLastResponseInfo when the last error is set to ERROR_INTERNET_EXTENDED_ERROR.
 * TODO:
 *     Add W-function tests.
 *     Add missing function tests:
 *         FtpCommand
 *         FtpFindFirstFile
 *         FtpGetCurrentDirectory
 *         FtpGetFileSize
 *         FtpSetCurrentDirectory
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wininet.h"
#include "winsock.h"

#include "wine/test.h"

static void test_getfile_no_open(void)
{
    BOOL      bRet;

    /* Invalid internet handle, the others are valid parameters */
    SetLastError(0xdeadbeef);
    bRet = FtpGetFileA(NULL, "welcome.msg", "should_be_non_existing_deadbeef", FALSE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_UNKNOWN, 0);
    ok ( bRet == FALSE, "Expected FtpGetFileA to fail\n");
    todo_wine
    ok ( GetLastError() == ERROR_INTERNET_NOT_INITIALIZED,
        "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());
}

static void test_connect(void)
{
    HINTERNET hInternet, hFtp;

    SetLastError(0xdeadbeef);
    hInternet = InternetOpen(NULL, 0, NULL, NULL, 0);
    ok(hInternet != NULL, "InternetOpen failed : %d\n", GetLastError());

    if(!hInternet)
    {
        skip("No internet connection could be made\n");
        return;
    }

    /* Try a few username/password combinations:
     * anonymous : NULL
     * NULL      : IEUser@
     * NULL      : NULL
     */

    SetLastError(0xdeadbeef);
    hFtp = InternetConnect(hInternet, "ftp.winehq.org", INTERNET_DEFAULT_FTP_PORT, "anonymous", NULL, INTERNET_SERVICE_FTP, 0, 0);
    ok ( hFtp == NULL, "Expected InternetConnect to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_LOGIN_FAILURE,
        "Expected ERROR_INTERNET_LOGIN_FAILURE, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    hFtp = InternetConnect(hInternet, "ftp.winehq.org", INTERNET_DEFAULT_FTP_PORT, NULL, "IEUser@", INTERNET_SERVICE_FTP, 0, 0);
    ok ( hFtp == NULL, "Expected InternetConnect to fail\n");
    ok ( GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Using a NULL username and password will be interpreted as anonymous ftp. The username will be 'anonymous' the password
     * is created via some simple heuristics (see dlls/wininet/ftp.c).
     * On Wine this registry key is not set by default so (NULL, NULL) will result in anonymous ftp with an (most likely) not
     * accepted password (the username).
     * If the first call fails because we get an ERROR_INTERNET_LOGIN_FAILURE, we try again with a (more) correct password.
     */

    SetLastError(0xdeadbeef);
    hFtp = InternetConnect(hInternet, "ftp.winehq.org", INTERNET_DEFAULT_FTP_PORT, NULL, NULL, INTERNET_SERVICE_FTP, 0, 0);
    if (!hFtp && (GetLastError() == ERROR_INTERNET_LOGIN_FAILURE))
    {
        /* We are most likely running on a clean Wine install or a Windows install where the registry key is removed */
        SetLastError(0xdeadbeef);
        hFtp = InternetConnect(hInternet, "ftp.winehq.org", INTERNET_DEFAULT_FTP_PORT, "anonymous", "IEUser@", INTERNET_SERVICE_FTP, 0, 0);
    }
    ok ( hFtp != NULL, "InternetConnect failed : %d\n", GetLastError());
    ok ( GetLastError() == ERROR_SUCCESS,
        "ERROR_SUCCESS, got %d\n", GetLastError());

    InternetCloseHandle(hFtp);
    InternetCloseHandle(hInternet);
}

static void test_createdir(void)
{
    BOOL      bRet;
    HINTERNET hInternet, hFtp, hConnect;

    /* Invalid internet handle, the other is a valid parameter */
    SetLastError(0xdeadbeef);
    bRet = FtpCreateDirectoryA(NULL, "new_directory_deadbeef");
    ok ( bRet == FALSE, "Expected FtpCreateDirectoryA to fail\n");
    ok ( GetLastError() == ERROR_INVALID_HANDLE,
        "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    hInternet = InternetOpen(NULL, 0, NULL, NULL, 0);
    hFtp = InternetConnect(hInternet, "ftp.winehq.org", INTERNET_DEFAULT_FTP_PORT, "anonymous", "IEUser@", INTERNET_SERVICE_FTP, 0, 0);
    if(!hFtp)
    {
        skip("No ftp connection could be made\n");
        InternetCloseHandle(hInternet);
        return;
    }

    /* We should have a ftp-connection (valid ftp session handle), try some creating */

    /* No directory-name */
    SetLastError(0xdeadbeef);
    bRet = FtpCreateDirectoryA(hFtp, NULL);
    ok ( bRet == FALSE, "Expected FtpCreateDirectoryA to fail\n");
    ok ( GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Parameters are OK, but we shouldn't be allowed to create the directory */
    SetLastError(0xdeadbeef);
    bRet = FtpCreateDirectoryA(hFtp, "new_directory_deadbeef");
    ok ( bRet == FALSE, "Expected FtpCreateDirectoryA to fail\n");
    todo_wine
    ok ( GetLastError() == ERROR_INTERNET_EXTENDED_ERROR,
        "Expected ERROR_INTERNET_EXTENDED_ERROR, got %d\n", GetLastError());

    InternetCloseHandle(hFtp);

    /* Http handle-type for ftp connection */

    hConnect = InternetConnect(hInternet, "www.winehq.org", INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);

    /* One small test to show that handle type is checked before parameters */
    SetLastError(0xdeadbeef);
    bRet = FtpCreateDirectoryA(hConnect, NULL);
    ok ( bRet == FALSE, "Expected FtpCreateDirectoryA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_INCORRECT_HANDLE_TYPE,
        "Expected ERROR_INTERNET_INCORRECT_HANDLE_TYPE, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    bRet = FtpCreateDirectoryA(hConnect, "new_directory_deadbeef");
    ok ( bRet == FALSE, "Expected FtpCreateDirectoryA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_INCORRECT_HANDLE_TYPE,
        "Expected ERROR_INTERNET_INCORRECT_HANDLE_TYPE, got %d\n", GetLastError());

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
}

static void test_deletefile(void)
{
    BOOL      bRet;
    HINTERNET hInternet, hFtp, hConnect;

    /* Invalid internet handle, the other is a valid parameter */
    SetLastError(0xdeadbeef);
    bRet = FtpDeleteFileA(NULL, "non_existent_file_deadbeef");
    ok ( bRet == FALSE, "Expected FtpDeleteFileA to fail\n");
    ok ( GetLastError() == ERROR_INVALID_HANDLE,
        "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    hInternet = InternetOpen(NULL, 0, NULL, NULL, 0);
    hFtp = InternetConnect(hInternet, "ftp.winehq.org", INTERNET_DEFAULT_FTP_PORT, "anonymous", "IEUser@", INTERNET_SERVICE_FTP, 0, 0);
    if(!hFtp)
    {
        skip("No ftp connection could be made\n");
        InternetCloseHandle(hInternet);
        return;
    }

    /* We should have a ftp-connection, try some deleting */

    /* No filename */
    SetLastError(0xdeadbeef);
    bRet = FtpDeleteFileA(hFtp, NULL);
    ok ( bRet == FALSE, "Expected FtpDeleteFileA to fail\n");
    ok ( GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Parameters are OK but remote file should not be there */
    SetLastError(0xdeadbeef);
    bRet = FtpDeleteFileA(hFtp, "non_existent_file_deadbeef");
    ok ( bRet == FALSE, "Expected FtpDeleteFileA to fail\n");
    todo_wine
    ok ( GetLastError() == ERROR_INTERNET_EXTENDED_ERROR,
        "Expected ERROR_INTERNET_EXTENDED_ERROR, got %d\n", GetLastError());

    InternetCloseHandle(hFtp);

    /* Http handle-type for ftp connection */

    hConnect = InternetConnect(hInternet, "www.winehq.org", INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);

    /* One small test to show that handle type is checked before parameters */
    SetLastError(0xdeadbeef);
    bRet = FtpDeleteFileA(hConnect, NULL);
    ok ( bRet == FALSE, "Expected FtpDeleteFileA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_INCORRECT_HANDLE_TYPE,
        "Expected ERROR_INTERNET_INCORRECT_HANDLE_TYPE, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    bRet = FtpDeleteFileA(hConnect, "non_existent_file_deadbeef");
    ok ( bRet == FALSE, "Expected FtpCreateDirectoryA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_INCORRECT_HANDLE_TYPE,
        "Expected ERROR_INTERNET_INCORRECT_HANDLE_TYPE, got %d\n", GetLastError());

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
}

static void test_getfile(void)
{
    BOOL      bRet;
    HINTERNET hInternet, hFtp, hConnect;

    /* Invalid internet handle, the other is a valid parameter */
    SetLastError(0xdeadbeef);
    bRet = FtpGetFileA(NULL, "welcome.msg", "should_be_non_existing_deadbeef", FALSE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_UNKNOWN, 0);
    ok ( bRet == FALSE, "Expected FtpGetFileA to fail\n");
    todo_wine
    ok ( GetLastError() == ERROR_INVALID_HANDLE,
        "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    /* Test to show that FtpGetFileA checks the parameters before the handle */
    SetLastError(0xdeadbeef);
    bRet = FtpGetFileA(NULL, NULL, NULL, FALSE, 0, 5, 0);
    ok ( bRet == FALSE, "Expected FtpGetFileA to fail\n");
    todo_wine
    ok ( GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    hInternet = InternetOpen(NULL, 0, NULL, NULL, 0);
    hFtp = InternetConnect(hInternet, "ftp.winehq.org", INTERNET_DEFAULT_FTP_PORT, "anonymous", "IEUser@", INTERNET_SERVICE_FTP, 0, 0);
    if(!hFtp)
    {
        skip("No ftp connection could be made\n");
        InternetCloseHandle(hInternet);
        return;
    }

    /* Make sure we start clean */

    DeleteFileA("should_be_non_existing_deadbeef");
    DeleteFileA("should_also_be_non_existing_deadbeef");

    /* We should have a ftp-connection, try some getting */

    /* No remote file */
    SetLastError(0xdeadbeef);
    bRet = FtpGetFileA(hFtp, NULL, "should_be_non_existing_deadbeef", FALSE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_UNKNOWN, 0);
    ok ( bRet == FALSE, "Expected FtpGetFileA to fail\n");
    todo_wine
    {
    ok ( GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    /* Currently Wine always creates the local file (even on failure) which is not correct, hence the test */
    ok (GetFileAttributesA("should_be_non_existing_deadbeef") == INVALID_FILE_ATTRIBUTES,
        "Local file should not have been created\n");
    }
    DeleteFileA("should_be_non_existing_deadbeef");

    /* No local file */
    SetLastError(0xdeadbeef);
    bRet = FtpGetFileA(hFtp, "welcome.msg", NULL, FALSE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_UNKNOWN, 0);
    ok ( bRet == FALSE, "Expected FtpGetFileA to fail\n");
    todo_wine
    ok ( GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Zero attributes, but call succeeds (as would CreateFile with zero attributes) */
    SetLastError(0xdeadbeef);
    bRet = FtpGetFileA(hFtp, "welcome.msg", "should_be_non_existing_deadbeef", FALSE, 0, FTP_TRANSFER_TYPE_UNKNOWN, 0);
    todo_wine
    {
    ok ( bRet == TRUE, "Expected FtpGetFileA to succeed\n");
    ok ( GetLastError() == ERROR_SUCCESS,
        "Expected ERROR_SUCCESS, got %d\n", GetLastError());
    }
    /* Wine passes this test but for the wrong reason */
    ok (GetFileAttributesA("should_be_non_existing_deadbeef") != INVALID_FILE_ATTRIBUTES,
        "Local file should have been created\n");
    DeleteFileA("should_be_non_existing_deadbeef");

    /* Illegal condition flags */
    SetLastError(0xdeadbeef);
    bRet = FtpGetFileA(hFtp, "welcome.msg", "should_be_non_existing_deadbeef", FALSE, FILE_ATTRIBUTE_NORMAL, 5, 0);
    ok ( bRet == FALSE, "Expected FtpGetFileA to fail\n");
    todo_wine
    {
    ok ( GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    ok (GetFileAttributesA("should_be_non_existing_deadbeef") == INVALID_FILE_ATTRIBUTES,
        "Local file should not have been created\n");
    }
    DeleteFileA("should_be_non_existing_deadbeef");

    /* Remote file doesn't exist */
    SetLastError(0xdeadbeef);
    bRet = FtpGetFileA(hFtp, "should_be_non_existing_deadbeef", "should_also_be_non_existing_deadbeef", FALSE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_UNKNOWN, 0);
    ok ( bRet == FALSE, "Expected FtpGetFileA to fail\n");
    todo_wine
    {
    ok ( GetLastError() == ERROR_INTERNET_EXTENDED_ERROR,
        "Expected ERROR_INTERNET_EXTENDED_ERROR, got %d\n", GetLastError());
    ok (GetFileAttributesA("should_also_be_non_existing_deadbeef") == INVALID_FILE_ATTRIBUTES,
        "Local file should not have been created\n");
    }
    DeleteFileA("should_also_be_non_existing_deadbeef");

    /* This one should succeed and give us a copy of the 'welcome.msg' file */
    SetLastError(0xdeadbeef);
    bRet = FtpGetFileA(hFtp, "welcome.msg", "should_be_non_existing_deadbeef", FALSE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_UNKNOWN, 0);
    todo_wine
    {
    ok ( bRet == TRUE, "Expected FtpGetFileA to succeed\n");
    ok ( GetLastError() == ERROR_SUCCESS,
        "Expected ERROR_SUCCESS, got %d\n", GetLastError());
    }

    if (GetFileAttributesA("should_be_non_existing_deadbeef") != INVALID_FILE_ATTRIBUTES)
    {
        /* Should succeed as fFailIfExists is set to FALSE (meaning don't fail if local file exists) */
        SetLastError(0xdeadbeef);
        bRet = FtpGetFileA(hFtp, "welcome.msg", "should_be_non_existing_deadbeef", FALSE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_UNKNOWN, 0);
        todo_wine
        {
        ok ( bRet == TRUE, "Expected FtpGetFileA to succeed\n");
        ok ( GetLastError() == ERROR_SUCCESS,
            "Expected ERROR_SUCCESS, got %d\n", GetLastError());
        }

        /* Should fail as fFailIfExists is set to TRUE */
        SetLastError(0xdeadbeef);
        bRet = FtpGetFileA(hFtp, "welcome.msg", "should_be_non_existing_deadbeef", TRUE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_UNKNOWN, 0);
        ok ( bRet == FALSE, "Expected FtpGetFileA to fail\n");
        todo_wine
        ok ( GetLastError() == ERROR_FILE_EXISTS,
            "Expected ERROR_FILE_EXISTS, got %d\n", GetLastError());

        /* Prove that the existence of the local file is checked first (or at least reported last) */
        SetLastError(0xdeadbeef);
        bRet = FtpGetFileA(hFtp, "should_be_non_existing_deadbeef", "should_be_non_existing_deadbeef", TRUE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_UNKNOWN, 0);
        ok ( bRet == FALSE, "Expected FtpGetFileA to fail\n");
        todo_wine
        ok ( GetLastError() == ERROR_FILE_EXISTS,
            "Expected ERROR_FILE_EXISTS, got %d\n", GetLastError());

        DeleteFileA("should_be_non_existing_deadbeef");
    }

    InternetCloseHandle(hFtp);

    /* Http handle-type for ftp connection */

    hConnect = InternetConnect(hInternet, "www.winehq.org", INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);

    /* One small test to show that handle type is checked before parameters */
    SetLastError(0xdeadbeef);
    bRet = FtpGetFileA(hConnect, "welcome.msg", "should_be_non_existing_deadbeef", FALSE, FILE_ATTRIBUTE_NORMAL, 5, 0);
    ok ( bRet == FALSE, "Expected FtpGetFileA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_INCORRECT_HANDLE_TYPE,
        "Expected ERROR_INTERNET_INCORRECT_HANDLE_TYPE, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    bRet = FtpGetFileA(hConnect, "should_be_non_existing_deadbeef", "should_be_non_existing_deadbeef", TRUE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_UNKNOWN, 0);
    ok ( bRet == FALSE, "Expected FtpGetFileA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_INCORRECT_HANDLE_TYPE,
        "Expected ERROR_INTERNET_INCORRECT_HANDLE_TYPE, got %d\n", GetLastError());

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
}

static void test_openfile(void)
{
    HINTERNET hOpenFile, hInternet, hFtp, hConnect;

    /* Invalid internet handle, the rest are valid parameters */
    SetLastError(0xdeadbeef);
    hOpenFile = FtpOpenFileA(NULL, "welcome.msg", GENERIC_READ, FTP_TRANSFER_TYPE_ASCII, 0);
    ok ( !hOpenFile, "Expected FtpOpenFileA to fail\n");
    todo_wine
    ok ( GetLastError() == ERROR_INVALID_HANDLE,
        "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());
    InternetCloseHandle(hOpenFile); /* Just in case */

    hInternet = InternetOpen(NULL, 0, NULL, NULL, 0);
    hFtp = InternetConnect(hInternet, "ftp.winehq.org", INTERNET_DEFAULT_FTP_PORT, "anonymous", "IEUser@", INTERNET_SERVICE_FTP, 0, 0);
    if(!hFtp)
    {
        skip("No ftp connection could be made\n");
        InternetCloseHandle(hInternet);
        return;
    }

    /* We should have a ftp-connection (valid ftp session handle), try some opening */

    /* No filename */
    SetLastError(0xdeadbeef);
    hOpenFile = FtpOpenFileA(hFtp, NULL, GENERIC_READ, FTP_TRANSFER_TYPE_ASCII, 0);
    ok ( !hOpenFile, "Expected FtpOpenFileA to fail\n");
    todo_wine
    ok ( GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    InternetCloseHandle(hOpenFile); /* Just in case */

    /* Illegal access flags */
    SetLastError(0xdeadbeef);
    hOpenFile = FtpOpenFileA(hFtp, "welcome.msg", 0, FTP_TRANSFER_TYPE_ASCII, 0);
    ok ( !hOpenFile, "Expected FtpOpenFileA to fail\n");
    todo_wine
    ok ( GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    InternetCloseHandle(hOpenFile); /* Just in case */

    /* Illegal combination of access flags */
    SetLastError(0xdeadbeef);
    hOpenFile = FtpOpenFileA(hFtp, "welcome.msg", GENERIC_READ|GENERIC_WRITE, FTP_TRANSFER_TYPE_ASCII, 0);
    ok ( !hOpenFile, "Expected FtpOpenFileA to fail\n");
    todo_wine
    ok ( GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    InternetCloseHandle(hOpenFile); /* Just in case */

    /* Illegal condition flags */
    SetLastError(0xdeadbeef);
    hOpenFile = FtpOpenFileA(hFtp, "welcome.msg", GENERIC_READ, 5, 0);
    todo_wine
    {
    ok ( !hOpenFile, "Expected FtpOpenFileA to fail\n");
    ok ( GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    }
    InternetCloseHandle(hOpenFile); /* Just in case */

    /* All OK */
    SetLastError(0xdeadbeef);
    hOpenFile = FtpOpenFileA(hFtp, "welcome.msg", GENERIC_READ, FTP_TRANSFER_TYPE_ASCII, 0);
    todo_wine
    {
    ok ( hOpenFile != NULL, "Expected FtpOpenFileA to succeed\n");
    /* For some strange/unknown reason, win98 returns ERROR_FILE_NOT_FOUND */
    ok ( GetLastError() == ERROR_SUCCESS || GetLastError() == ERROR_FILE_NOT_FOUND,
        "Expected ERROR_SUCCESS or ERROR_FILE_NOT_FOUND (win98), got %d\n", GetLastError());
    }

    if (hOpenFile)
    {
        BOOL bRet;

        /* We have a handle so all ftp calls should fail (TODO: Put more ftp-calls in here) */
        SetLastError(0xdeadbeef);
        bRet = FtpGetFileA(hFtp, "welcome.msg", "should_be_non_existing_deadbeef", FALSE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_UNKNOWN, 0);
        ok ( bRet == FALSE, "Expected FtpDeleteFileA to fail\n");
        ok ( GetLastError() == ERROR_FTP_TRANSFER_IN_PROGRESS,
            "Expected ERROR_FTP_TRANSFER_IN_PROGRESS, got %d\n", GetLastError());
        DeleteFileA("should_be_non_existing_deadbeef"); /* Just in case */
    }

    InternetCloseHandle(hOpenFile);
    InternetCloseHandle(hFtp);

    /* Http handle-type for ftp connection */

    hConnect = InternetConnect(hInternet, "www.winehq.org", INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);

    /* One small test to show that handle type is checked before parameters */
    SetLastError(0xdeadbeef);
    hOpenFile = FtpOpenFileA(hConnect, "welcome.msg", GENERIC_READ, 5, 0);
    ok ( !hOpenFile, "Expected FtpOpenFileA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_INCORRECT_HANDLE_TYPE,
        "Expected ERROR_INTERNET_INCORRECT_HANDLE_TYPE, got %d\n", GetLastError());
    InternetCloseHandle(hOpenFile); /* Just in case */

    SetLastError(0xdeadbeef);
    hOpenFile = FtpOpenFileA(hConnect, "welcome.msg", GENERIC_READ, FTP_TRANSFER_TYPE_ASCII, 0);
    ok ( hOpenFile == NULL, "Expected FtpOpenFileA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_INCORRECT_HANDLE_TYPE,
        "Expected ERROR_INTERNET_INCORRECT_HANDLE_TYPE, got %d\n", GetLastError());

    InternetCloseHandle(hOpenFile); /* Just in case */
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
}

static void test_putfile(void)
{
    BOOL      bRet;
    HINTERNET hInternet, hFtp, hConnect;
    HANDLE    hFile;

    /* Invalid internet handle, the rest are valid parameters */
    SetLastError(0xdeadbeef);
    bRet = FtpPutFileA(NULL, "non_existing_local", "non_existing_remote", FTP_TRANSFER_TYPE_UNKNOWN, 0);
    ok ( bRet == FALSE, "Expected FtpPutFileA to fail\n");
    todo_wine
    ok ( GetLastError() == ERROR_INVALID_HANDLE,
        "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    hInternet = InternetOpen(NULL, 0, NULL, NULL, 0);
    hFtp = InternetConnect(hInternet, "ftp.winehq.org", INTERNET_DEFAULT_FTP_PORT, "anonymous", "IEUser@", INTERNET_SERVICE_FTP, 0, 0);
    if(!hFtp)
    {
        skip("No ftp connection could be made\n");
        InternetCloseHandle(hInternet);
        return;
    }

    /* We should have a ftp-connection, try some puts */

    /* No local file given */
    SetLastError(0xdeadbeef);
    bRet = FtpPutFileA(hFtp, NULL, "non_existing_remote", FTP_TRANSFER_TYPE_UNKNOWN, 0);
    ok ( bRet == FALSE, "Expected FtpPutFileA to fail\n");
    ok ( GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* No remote file given */
    SetLastError(0xdeadbeef);
    bRet = FtpPutFileA(hFtp, "non_existing_local", NULL, FTP_TRANSFER_TYPE_UNKNOWN, 0);
    ok ( bRet == FALSE, "Expected FtpPutFileA to fail\n");
    ok ( GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Illegal condition flags */
    SetLastError(0xdeadbeef);
    bRet = FtpPutFileA(hFtp, "non_existing_local", "non_existing_remote", 5, 0);
    ok ( bRet == FALSE, "Expected FtpPutFileA to fail\n");
    todo_wine
    ok ( GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Parameters are OK but local file doesn't exist */
    SetLastError(0xdeadbeef);
    bRet = FtpPutFileA(hFtp, "non_existing_local", "non_existing_remote", FTP_TRANSFER_TYPE_UNKNOWN, 0);
    ok ( bRet == FALSE, "Expected FtpPutFileA to fail\n");
    todo_wine
    ok ( GetLastError() == ERROR_FILE_NOT_FOUND,
        "Expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());

    /* Create a temporary local file */
    SetLastError(0xdeadbeef);
    hFile = CreateFileA("now_existing_local", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok ( hFile != NULL, "Error creating a local file : %d\n", GetLastError());
    CloseHandle(hFile);

    /* Local file exists but we shouldn't be allowed to 'put' the file */
    SetLastError(0xdeadbeef);
    bRet = FtpPutFileA(hFtp, "now_existing_local", "non_existing_remote", FTP_TRANSFER_TYPE_UNKNOWN, 0);
    ok ( bRet == FALSE, "Expected FtpPutFileA to fail\n");
    todo_wine
    ok ( GetLastError() == ERROR_INTERNET_EXTENDED_ERROR,
        "Expected ERROR_INTERNET_EXTENDED_ERROR, got %d\n", GetLastError());

    DeleteFileA("now_existing_local");

    InternetCloseHandle(hFtp);

    /* Http handle-type for ftp connection */

    hConnect = InternetConnect(hInternet, "www.winehq.org", INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);

    /* One small test to show that handle type is checked before parameters */
    SetLastError(0xdeadbeef);
    bRet = FtpPutFileA(hConnect, "non_existing_local", "non_existing_remote", 5, 0);
    ok ( bRet == FALSE, "Expected FtpPutFileA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_INCORRECT_HANDLE_TYPE,
        "Expected ERROR_INTERNET_INCORRECT_HANDLE_TYPE, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    bRet = FtpPutFileA(hConnect, "non_existing_local", "non_existing_remote", FTP_TRANSFER_TYPE_UNKNOWN, 0);
    ok ( bRet == FALSE, "Expected FtpPutFileA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_INCORRECT_HANDLE_TYPE,
        "Expected ERROR_INTERNET_INCORRECT_HANDLE_TYPE, got %d\n", GetLastError());

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
}

static void test_removedir(void)
{
    BOOL      bRet;
    HINTERNET hInternet, hFtp, hConnect;

    /* Invalid internet handle, the other is a valid parameter */
    SetLastError(0xdeadbeef);
    bRet = FtpRemoveDirectoryA(NULL, "should_be_non_existing_deadbeef_dir");
    ok ( bRet == FALSE, "Expected FtpRemoveDirectoryA to fail\n");
    ok ( GetLastError() == ERROR_INVALID_HANDLE,
        "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    hInternet = InternetOpen(NULL, 0, NULL, NULL, 0);
    hFtp = InternetConnect(hInternet, "ftp.winehq.org", INTERNET_DEFAULT_FTP_PORT, "anonymous", "IEUser@", INTERNET_SERVICE_FTP, 0, 0);
    if(!hFtp)
    {
        skip("No ftp connection could be made\n");
        InternetCloseHandle(hInternet);
        return;
    }

    /* We should have a ftp-connection, try some removing */

    /* No remote directory given */
    SetLastError(0xdeadbeef);
    bRet = FtpRemoveDirectoryA(hFtp, NULL);
    ok ( bRet == FALSE, "Expected FtpRemoveDirectoryA to fail\n");
    ok ( GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Remote directory doesn't exist */
    SetLastError(0xdeadbeef);
    bRet = FtpRemoveDirectoryA(hFtp, "should_be_non_existing_deadbeef_dir");
    ok ( bRet == FALSE, "Expected FtpRemoveDirectoryA to fail\n");
    todo_wine
    ok ( GetLastError() == ERROR_INTERNET_EXTENDED_ERROR,
        "Expected ERROR_INTERNET_EXTENDED_ERROR, got %d\n", GetLastError());

    /* We shouldn't be allowed to remove that directory */
    SetLastError(0xdeadbeef);
    bRet = FtpRemoveDirectoryA(hFtp, "pub");
    ok ( bRet == FALSE, "Expected FtpRemoveDirectoryA to fail\n");
    todo_wine
    ok ( GetLastError() == ERROR_INTERNET_EXTENDED_ERROR,
        "Expected ERROR_INTERNET_EXTENDED_ERROR, got %d\n", GetLastError());

    InternetCloseHandle(hFtp);

    /* Http handle-type for ftp connection */

    hConnect = InternetConnect(hInternet, "www.winehq.org", INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);

    /* One small test to show that handle type is checked before parameters */
    SetLastError(0xdeadbeef);
    bRet = FtpRemoveDirectoryA(hConnect, NULL);
    ok ( bRet == FALSE, "Expected FtpRemoveDirectoryA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_INCORRECT_HANDLE_TYPE,
        "Expected ERROR_INTERNET_INCORRECT_HANDLE_TYPE, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    bRet = FtpRemoveDirectoryA(hConnect, "should_be_non_existing_deadbeef_dir");
    ok ( bRet == FALSE, "Expected FtpRemoveDirectoryA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_INCORRECT_HANDLE_TYPE,
        "Expected ERROR_INTERNET_INCORRECT_HANDLE_TYPE, got %d\n", GetLastError());

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
}

static void test_renamefile(void)
{
    BOOL      bRet;
    HINTERNET hInternet, hFtp, hConnect;

    /* Invalid internet handle, the rest are valid parameters */
    SetLastError(0xdeadbeef);
    bRet = FtpRenameFileA(NULL , "should_be_non_existing_deadbeef", "new");
    ok ( bRet == FALSE, "Expected FtpRenameFileA to fail\n");
    ok ( GetLastError() == ERROR_INVALID_HANDLE,
        "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    hInternet = InternetOpen(NULL, 0, NULL, NULL, 0);
    hFtp = InternetConnect(hInternet, "ftp.winehq.org", INTERNET_DEFAULT_FTP_PORT, "anonymous", "IEUser@", INTERNET_SERVICE_FTP, 0, 0);
    if(!hFtp)
    {
        skip("No ftp connection could be made\n");
        InternetCloseHandle(hInternet);
        return;
    }

    /* We should have a ftp-connection, try some renaming */

    /* No 'existing' file */
    SetLastError(0xdeadbeef);
    bRet = FtpRenameFileA(hFtp , NULL, "new");
    ok ( bRet == FALSE, "Expected FtpRenameFileA to fail\n");
    ok ( GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* No new file */
    SetLastError(0xdeadbeef);
    bRet = FtpRenameFileA(hFtp , "should_be_non_existing_deadbeef", NULL);
    ok ( bRet == FALSE, "Expected FtpRenameFileA to fail\n");
    ok ( GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Existing file shouldn't be there */
    SetLastError(0xdeadbeef);
    bRet = FtpRenameFileA(hFtp , "should_be_non_existing_deadbeef", "new");
    ok ( bRet == FALSE, "Expected FtpRenameFileA to fail\n");
    todo_wine
    ok ( GetLastError() == ERROR_INTERNET_EXTENDED_ERROR,
        "Expected ERROR_INTERNET_EXTENDED_ERROR, got %d\n", GetLastError());

    InternetCloseHandle(hFtp);

    /* Http handle-type for ftp connection */

    hConnect = InternetConnect(hInternet, "www.winehq.org", INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);

    /* One small test to show that handle type is checked before parameters */
    SetLastError(0xdeadbeef);
    bRet = FtpRenameFileA(hConnect , "should_be_non_existing_deadbeef", NULL);
    ok ( bRet == FALSE, "Expected FtpRenameFileA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_INCORRECT_HANDLE_TYPE,
        "Expected ERROR_INTERNET_INCORRECT_HANDLE_TYPE, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    bRet = FtpRenameFileA(hConnect , "should_be_non_existing_deadbeef", "new");
    ok ( bRet == FALSE, "Expected FtpRenameFileA to fail\n");
    ok ( GetLastError() == ERROR_INTERNET_INCORRECT_HANDLE_TYPE,
        "Expected ERROR_INTERNET_INCORRECT_HANDLE_TYPE, got %d\n", GetLastError());

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
}

START_TEST(ftp)
{
    /* The first call should always be a proper InternetOpen, if not
     * several calls will return ERROR_INTERNET_NOT_INITIALIZED when
     * all parameters are correct but no session handle is given. Whereas
     * the same call will return ERROR_INVALID_HANDLE if an InternetOpen
     * is done before.
     * The following test will show that behaviour, where the tests inside
     * the other sub-tests will show the other situation.
     */
    test_getfile_no_open();
    test_connect();
    test_createdir();
    test_deletefile();
    test_getfile();
    test_openfile();
    test_putfile();
    test_removedir();
    test_renamefile();
}

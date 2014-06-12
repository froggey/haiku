/*
 * Copyright 2004-2012, Haiku Inc. All rights reserved.
 * Copyright 2001-2003 Dr. Zoidberg Enterprises. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


//!	The mail daemon's settings


#include <MailSettings.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <MailDaemon.h>
#include <Message.h>
#include <Messenger.h>
#include <Path.h>
#include <String.h>
#include <Window.h>

#include <MailPrivate.h>


//	#pragma mark - BMailSettings


BMailSettings::BMailSettings()
{
	Reload();
}


BMailSettings::~BMailSettings()
{
}


status_t
BMailSettings::InitCheck() const
{
	return B_OK;
}


status_t
BMailSettings::Save()
{
	BPath path;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status != B_OK) {
		fprintf(stderr, "Couldn't find user settings directory: %s\n",
			strerror(status));
		return status;
	}

	path.Append("Mail");

	status = BPrivate::WriteMessageFile(fData, path, "new_mail_daemon");
	if (status != B_OK)
		return status;

	BMessenger(B_MAIL_DAEMON_SIGNATURE).SendMessage(
		BPrivate::kMsgSettingsUpdated);
	return B_OK;
}


status_t
BMailSettings::Reload()
{
	BPath path;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status != B_OK) {
		fprintf(stderr, "Couldn't read settings from '%s': %s\n",
			path.Path(), strerror(status));
		return status;
	}

	path.Append("Mail/new_mail_daemon");
	BFile file(path.Path(), B_READ_ONLY);
	status = file.InitCheck();
	if (status != B_OK) {
		fprintf(stderr, "Couldn't read settings from '%s': %s\n",
			path.Path(), strerror(status));
		return status;
	}

	// read settings
	BMessage settings;
	status = settings.Unflatten(&file);
	if (status != B_OK) {
		fprintf(stderr, "Couldn't read settings from '%s': %s\n",
			path.Path(), strerror(status));
		return status;
	}

	// clobber old settings
	fData = settings;
	return B_OK;
}


//	# pragma mark - Global settings


uint32
BMailSettings::ShowStatusWindow()
{
	int32 showStatusWindow;
	if (fData.FindInt32("ShowStatusWindow", &showStatusWindow) != B_OK) {
		// show during send and receive
		return B_MAIL_SHOW_STATUS_WINDOW_WHEN_ACTIVE;
	}

	return showStatusWindow;
}


status_t
BMailSettings::SetShowStatusWindow(uint32 mode)
{
	return fData.SetInt32("ShowStatusWindow", mode);
}


bool
BMailSettings::DaemonAutoStarts()
{
	return fData.FindBool("DaemonAutoStarts");
}


status_t
BMailSettings::SetDaemonAutoStarts(bool startIt)
{
	return fData.SetBool("DaemonAutoStarts", startIt);
}


bigtime_t
BMailSettings::AutoCheckInterval()
{
	bigtime_t value;
	if (fData.FindInt64("AutoCheckInterval", &value) != B_OK) {
		// every 5 min
		return 5 * 60 * 1000 * 1000;
	}
	return value;
}


status_t
BMailSettings::SetAutoCheckInterval(bigtime_t interval)
{
	return fData.SetInt64("AutoCheckInterval", interval);
}


int32
BMailSettings::DefaultOutboundAccount()
{
	return fData.FindInt32("DefaultOutboundAccount");
}


status_t
BMailSettings::SetDefaultOutboundAccount(int32 to)
{
	return fData.SetInt32("DefaultOutboundAccount", to);
}


// #pragma mark -


BMailAccounts::BMailAccounts()
{
	BPath path;
	status_t status = AccountsPath(path);
	if (status != B_OK)
		return;

	BDirectory dir(path.Path());
	if (dir.InitCheck() != B_OK)
		return;

	std::vector<time_t> creationTimeList;
	BEntry entry;
	while (dir.GetNextEntry(&entry) != B_ENTRY_NOT_FOUND) {
		BNode node(&entry);
		time_t creationTime;
		if (node.GetCreationTime(&creationTime) != B_OK)
			continue;

		BMailAccountSettings* account = new BMailAccountSettings(entry);
		if (account->InitCheck() != B_OK) {
			delete account;
			continue;
		}

		// sort by creation time
		int insertIndex = -1;
		for (unsigned int i = 0; i < creationTimeList.size(); i++) {
			if (creationTimeList[i] > creationTime) {
				insertIndex = i;
				break;
			}
		}
		if (insertIndex < 0) {
			fAccounts.AddItem(account);
			creationTimeList.push_back(creationTime);
		} else {
			fAccounts.AddItem(account, insertIndex);
			creationTimeList.insert(creationTimeList.begin() + insertIndex,
				creationTime);
		}
	}
}


status_t
BMailAccounts::AccountsPath(BPath& path)
{
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status != B_OK)
		return status;
	return path.Append("Mail/accounts");
}


BMailAccounts::~BMailAccounts()
{
	for (int i = 0; i < fAccounts.CountItems(); i++)
		delete fAccounts.ItemAt(i);
}


int32
BMailAccounts::CountAccounts()
{
	return fAccounts.CountItems();
}


BMailAccountSettings*
BMailAccounts::AccountAt(int32 index)
{
	return fAccounts.ItemAt(index);
}


BMailAccountSettings*
BMailAccounts::AccountByID(int32 id)
{
	for (int i = 0; i < fAccounts.CountItems(); i++) {
		BMailAccountSettings* account = fAccounts.ItemAt(i);
		if (account->AccountID() == id)
			return account;
	}
	return NULL;
}


BMailAccountSettings*
BMailAccounts::AccountByName(const char* name)
{
	for (int i = 0; i < fAccounts.CountItems(); i++) {
		BMailAccountSettings* account = fAccounts.ItemAt(i);
		if (strcmp(account->Name(), name) == 0)
			return account;
	}
	return NULL;
}


// #pragma mark -


BMailAddOnSettings::BMailAddOnSettings()
{
}


BMailAddOnSettings::~BMailAddOnSettings()
{
}


status_t
BMailAddOnSettings::Load(const BMessage& message)
{
	const char* pathString = NULL;
	if (message.FindString("add-on path", &pathString) != B_OK)
		return B_BAD_VALUE;

	BPath path(pathString);
	if (!path.IsAbsolute()) {
		directory_which which[] = {
			B_USER_ADDONS_DIRECTORY,
			/*B_COMMON_ADDONS_DIRECTORY,*/
			B_SYSTEM_ADDONS_DIRECTORY
		};

		for (size_t i = 0; i < sizeof(which) / sizeof(which[0]); i++) {
			status_t status = find_directory(which[i], &path);
			if (status != B_OK)
				continue;

			path.Append("mail_daemon");
			path.Append(pathString);

			if (BEntry(path.Path()).Exists())
				break;
		}
	}

	status_t status = get_ref_for_path(path.Path(), &fRef);
	if (status != B_OK)
		return status;

	BMessage settings;
	message.FindMessage("settings", &settings);

	MakeEmpty();
	Append(settings);

	fOriginalSettings = *this;
	fOriginalRef = fRef;
	return B_OK;
}


status_t
BMailAddOnSettings::Save(BMessage& message)
{
	BPath path(&fRef);
	status_t status = message.AddString("add-on path", _RelativizePath(path));
	if (status == B_OK)
		status = message.AddMessage("settings", this);
	if (status != B_OK)
		return status;

	fOriginalSettings = *this;
	fOriginalRef = fRef;
	return B_OK;
}


void
BMailAddOnSettings::SetAddOnRef(const entry_ref& ref)
{
	fRef = ref;
}


const entry_ref&
BMailAddOnSettings::AddOnRef() const
{
	return fRef;
}


bool
BMailAddOnSettings::HasBeenModified() const
{
	return fRef != fOriginalRef
		|| !fOriginalSettings.HasSameData(*this, true, true);
}


/*!	Cuts off the ".../add-ons/mail_daemon" part of the provided \a path
	in case it exists. Otherwise, the complete path will be returned.
*/
const char*
BMailAddOnSettings::_RelativizePath(const BPath& path) const
{
	const char* string = path.Path();
	const char* parentDirectory = "/mail_daemon/";
	const char* at = strstr(string, parentDirectory);
	if (at == NULL)
		return string;

	return at + strlen(parentDirectory);
}


// #pragma mark -


BMailProtocolSettings::BMailProtocolSettings()
	:
	fFiltersSettings(5, true)
{
}


BMailProtocolSettings::~BMailProtocolSettings()
{
}


status_t
BMailProtocolSettings::Load(const BMessage& message)
{
	status_t status = BMailAddOnSettings::Load(message);
	if (status != B_OK)
		return status;

	type_code typeFound;
	int32 countFound;
	message.GetInfo("filters", &typeFound, &countFound);
	if (typeFound != B_MESSAGE_TYPE)
		return B_BAD_VALUE;

	for (int i = 0; i < countFound; i++) {
		int32 index = AddFilterSettings();
		if (index < 0)
			return B_NO_MEMORY;

		BMailAddOnSettings* filterSettings = fFiltersSettings.ItemAt(index);

		BMessage filterMessage;
		message.FindMessage("filters", i, &filterMessage);
		if (filterSettings->Load(filterMessage) != B_OK)
			RemoveFilterSettings(index);
	}
	return B_OK;
}


status_t
BMailProtocolSettings::Save(BMessage& message)
{
	status_t status = BMailAddOnSettings::Save(message);
	if (status != B_OK)
		return status;

	for (int i = 0; i < CountFilterSettings(); i++) {
		BMessage filter;
		BMailAddOnSettings* filterSettings = fFiltersSettings.ItemAt(i);
		filterSettings->Save(filter);
		message.AddMessage("filters", &filter);
	}
	return B_OK;
}


int32
BMailProtocolSettings::CountFilterSettings() const
{
	return fFiltersSettings.CountItems();
}


int32
BMailProtocolSettings::AddFilterSettings(const entry_ref* ref)
{
	BMailAddOnSettings* filterSettings = new BMailAddOnSettings();
	if (ref != NULL)
		filterSettings->SetAddOnRef(*ref);

	if (fFiltersSettings.AddItem(filterSettings))
		return fFiltersSettings.CountItems() - 1;

	delete filterSettings;
	return -1;
}


void
BMailProtocolSettings::RemoveFilterSettings(int32 index)
{
	fFiltersSettings.RemoveItemAt(index);
}


bool
BMailProtocolSettings::MoveFilterSettings(int32 from, int32 to)
{
	if (from < 0 || from >= (int32)CountFilterSettings() || to < 0
		|| to >= (int32)CountFilterSettings())
		return false;
	if (from == to)
		return true;

	BMailAddOnSettings* settings = fFiltersSettings.RemoveItemAt(from);
	fFiltersSettings.AddItem(settings, to);
	return true;
}


BMailAddOnSettings*
BMailProtocolSettings::FilterSettingsAt(int32 index) const
{
	return fFiltersSettings.ItemAt(index);
}


bool
BMailProtocolSettings::HasBeenModified() const
{
	if (BMailAddOnSettings::HasBeenModified())
		return true;
	for (int32 i = 0; i < CountFilterSettings(); i++) {
		if (FilterSettingsAt(i)->HasBeenModified())
			return true;
	}
	return false;
}


//	#pragma mark -


BMailAccountSettings::BMailAccountSettings()
	:
	fStatus(B_OK),
	fInboundEnabled(true),
	fOutboundEnabled(true),
	fModified(true)
{
	fAccountID = real_time_clock();
}


BMailAccountSettings::BMailAccountSettings(BEntry account)
	:
	fAccountFile(account),
	fModified(false)
{
	fStatus = Reload();
}


BMailAccountSettings::~BMailAccountSettings()
{

}


void
BMailAccountSettings::SetAccountID(int32 id)
{
	fModified = true;
	fAccountID = id;
}


int32
BMailAccountSettings::AccountID() const
{
	return fAccountID;
}


void
BMailAccountSettings::SetName(const char* name)
{
	fModified = true;
	fAccountName = name;
}


const char*
BMailAccountSettings::Name() const
{
	return fAccountName;
}


void
BMailAccountSettings::SetRealName(const char* realName)
{
	fModified = true;
	fRealName = realName;
}


const char*
BMailAccountSettings::RealName() const
{
	return fRealName;
}


void
BMailAccountSettings::SetReturnAddress(const char* returnAddress)
{
	fModified = true;
	fReturnAdress = returnAddress;
}


const char*
BMailAccountSettings::ReturnAddress() const
{
	return fReturnAdress;
}


bool
BMailAccountSettings::SetInboundAddOn(const char* name)
{
	BPath path;
	status_t status = find_directory(B_BEOS_ADDONS_DIRECTORY, &path);
	if (status != B_OK)
		return false;
	path.Append("mail_daemon");
	path.Append("inbound_protocols");
	path.Append(name);
	entry_ref ref;
	get_ref_for_path(path.Path(), &ref);
	fInboundSettings.SetAddOnRef(ref);

	return true;
}


bool
BMailAccountSettings::SetOutboundAddOn(const char* name)
{
	BPath path;
	status_t status = find_directory(B_BEOS_ADDONS_DIRECTORY, &path);
	if (status != B_OK)
		return false;
	path.Append("mail_daemon");
	path.Append("outbound_protocols");
	path.Append(name);
	entry_ref ref;
	get_ref_for_path(path.Path(), &ref);
	fOutboundSettings.SetAddOnRef(ref);

	return true;
}


const entry_ref&
BMailAccountSettings::InboundAddOnRef() const
{
	return fInboundSettings.AddOnRef();
}


const entry_ref&
BMailAccountSettings::OutboundAddOnRef() const
{
	return fOutboundSettings.AddOnRef();
}


BMailProtocolSettings&
BMailAccountSettings::InboundSettings()
{
	return fInboundSettings;
}


const BMailProtocolSettings&
BMailAccountSettings::InboundSettings() const
{
	return fInboundSettings;
}


BMailProtocolSettings&
BMailAccountSettings::OutboundSettings()
{
	return fOutboundSettings;
}


const BMailProtocolSettings&
BMailAccountSettings::OutboundSettings() const
{
	return fOutboundSettings;
}


bool
BMailAccountSettings::HasInbound()
{
	return BEntry(&fInboundSettings.AddOnRef()).Exists();
}


bool
BMailAccountSettings::HasOutbound()
{
	return BEntry(&fOutboundSettings.AddOnRef()).Exists();
}


void
BMailAccountSettings::SetInboundEnabled(bool enabled)
{
	fInboundEnabled = enabled;
	fModified = true;
}


bool
BMailAccountSettings::IsInboundEnabled() const
{
	return fInboundEnabled;
}


void
BMailAccountSettings::SetOutboundEnabled(bool enabled)
{
	fOutboundEnabled = enabled;
	fModified = true;
}


bool
BMailAccountSettings::IsOutboundEnabled() const
{
	return fOutboundEnabled;
}


status_t
BMailAccountSettings::Reload()
{
	BFile file(&fAccountFile, B_READ_ONLY);
	status_t status = file.InitCheck();
	if (status != B_OK)
		return status;
	BMessage settings;
	settings.Unflatten(&file);

	int32 id;
	if (settings.FindInt32("id", &id) == B_OK)
		fAccountID = id;
	settings.FindString("name", &fAccountName);
	settings.FindString("real_name", &fRealName);
	settings.FindString("return_address", &fReturnAdress);

	BMessage inboundSettings;
	settings.FindMessage("inbound", &inboundSettings);
	fInboundSettings.Load(inboundSettings);
	BMessage outboundSettings;
	settings.FindMessage("outbound", &outboundSettings);
	fOutboundSettings.Load(outboundSettings);

	if (settings.FindBool("inbound_enabled", &fInboundEnabled) != B_OK)
		fInboundEnabled = true;
	if (settings.FindBool("outbound_enabled", &fOutboundEnabled) != B_OK)
		fOutboundEnabled = true;

	fModified = false;
	return B_OK;
}


status_t
BMailAccountSettings::Save()
{
	fModified = false;

	BMessage settings;
	settings.AddInt32("id", fAccountID);
	settings.AddString("name", fAccountName);
	settings.AddString("real_name", fRealName);
	settings.AddString("return_address", fReturnAdress);

	BMessage inboundSettings;
	fInboundSettings.Save(inboundSettings);
	settings.AddMessage("inbound", &inboundSettings);
	BMessage outboundSettings;
	fOutboundSettings.Save(outboundSettings);
	settings.AddMessage("outbound", &outboundSettings);

	settings.AddBool("inbound_enabled", fInboundEnabled);
	settings.AddBool("outbound_enabled", fOutboundEnabled);

	status_t status = _CreateAccountFilePath();
	if (status != B_OK)
		return status;

	BFile file(&fAccountFile, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	status = file.InitCheck();
	if (status != B_OK)
		return status;
	return settings.Flatten(&file);
}


status_t
BMailAccountSettings::Delete()
{
	return fAccountFile.Remove();
}


bool
BMailAccountSettings::HasBeenModified() const
{
	return fModified
		|| fInboundSettings.HasBeenModified()
		|| fOutboundSettings.HasBeenModified();
}


const BEntry&
BMailAccountSettings::AccountFile() const
{
	return fAccountFile;
}


status_t
BMailAccountSettings::_CreateAccountFilePath()
{
	BPath path;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status != B_OK)
		return status;
	path.Append("Mail/accounts");
	create_directory(path.Path(), 777);

	if (fAccountFile.InitCheck() == B_OK)
		return B_OK;

	BString fileName = fAccountName;
	if (fileName == "")
		fileName << fAccountID;
	for (int i = 0; ; i++) {
		BString testFileName = fileName;
		if (i != 0) {
			testFileName += "_";
			testFileName << i;
		}
		BPath testPath(path);
		testPath.Append(testFileName);
		BEntry testEntry(testPath.Path());
		if (!testEntry.Exists()) {
			fileName = testFileName;
			break;
		}
	}

	path.Append(fileName);
	return fAccountFile.SetTo(path.Path());
}

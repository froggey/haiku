/*
 * Copyright 2007-2011, Haiku, Inc. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include "AutoConfigView.h"

#include <pwd.h>

#include <Catalog.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Message.h>
#include <MenuItem.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <String.h>
#include <Window.h>

#include <MailSettings.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "E-Mail"


AutoConfigView::AutoConfigView(BRect rect, AutoConfig &config)
	:
	BBox(rect),
	fAutoConfig(config)
{
	int32 stepSize = 30;
	int32 divider = 100;
	BPoint topLeft(20, 20);
	BPoint rightDown(rect.Width() - 20, 20 + stepSize);

	// protocol view
	topLeft.y += stepSize;
	rightDown.y += stepSize;
	fInProtocolsField = _SetupProtocolView(BRect(topLeft, rightDown));
	if (fInProtocolsField)
		AddChild(fInProtocolsField);

	// search for smtp ref
	_GetSMTPAddonRef(&fSMTPAddonRef);

	// email view
	topLeft.y += stepSize;
	rightDown.y += stepSize;
	fEmailView = new BTextControl(BRect(topLeft, rightDown), "email",
		B_TRANSLATE("E-mail address:"), "", new BMessage(kEMailChangedMsg));
	fEmailView->SetDivider(divider);
	AddChild(fEmailView);

	// login name view
	topLeft.y += stepSize;
	rightDown.y += stepSize;
	fLoginNameView = new BTextControl(BRect(topLeft, rightDown),
		"login", B_TRANSLATE("Login name:"), "", NULL);
	fLoginNameView->SetDivider(divider);
	AddChild(fLoginNameView);

	// password view
	topLeft.y += stepSize;
	rightDown.y += stepSize;
	fPasswordView = new BTextControl(BRect(topLeft, rightDown), "password",
		B_TRANSLATE("Password:"), "", NULL);
	fPasswordView->SetDivider(divider);
	fPasswordView->TextView()->HideTyping(true);
	AddChild(fPasswordView);

	// account view
	topLeft.y += stepSize;
	rightDown.y += stepSize;
	fAccountNameView = new BTextControl(BRect(topLeft, rightDown), "account",
		B_TRANSLATE("Account name:"), "", NULL);
	fAccountNameView->SetDivider(divider);
	AddChild(fAccountNameView);

	// name view
	topLeft.y += stepSize;
	rightDown.y += stepSize;
	fNameView = new BTextControl(BRect(topLeft, rightDown), "name",
		B_TRANSLATE("Real name:"), "", NULL);
	AddChild(fNameView);
	fNameView->SetDivider(divider);

	struct passwd* passwd = getpwent();
	if (passwd != NULL)
		fNameView->SetText(passwd->pw_gecos);
}


void
AutoConfigView::AttachedToWindow()
{
	fEmailView->SetTarget(this);
	fEmailView->MakeFocus(true);
}


void
AutoConfigView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case kEMailChangedMsg:
		{
			BString text = fLoginNameView->Text();
			if (text == "")
				_ProposeUsername();
			fLoginNameView->MakeFocus();
			fLoginNameView->TextView()->SelectAll();

			text = fAccountNameView->Text();
			if (text == "")
				fAccountNameView->SetText(fEmailView->Text());
			break;
		}
		default:
			BView::MessageReceived(msg);
			break;
	}
}


bool
AutoConfigView::GetBasicAccountInfo(account_info &info)
{
	status_t status = B_OK;

	BString inboundProtocolName = "";
	BMenuItem* item = fInProtocolsField->Menu()->FindMarked();
	if (item) {
		inboundProtocolName = item->Label();
		item->Message()->FindRef("protocol", &(info.inboundProtocol));
	}
	else
		status = B_ERROR;

	if (inboundProtocolName.FindFirst("IMAP") >= 0)
		info.inboundType = IMAP;
	else
		info.inboundType = POP;

	info.outboundProtocol = fSMTPAddonRef;
	info.name = fNameView->Text();
	info.accountName = fAccountNameView->Text();
	info.email = fEmailView->Text();
	info.loginName = fLoginNameView->Text();
	info.password = fPasswordView->Text();

	return status;
}


BMenuField*
AutoConfigView::_SetupProtocolView(BRect rect)
{
	BPopUpMenu* menu = new BPopUpMenu(B_TRANSLATE("Choose Protocol"));

	for (int i = 0; i < 2; i++) {
		BPath path;
		status_t status = find_directory((i == 0) ? B_USER_ADDONS_DIRECTORY :
			B_BEOS_ADDONS_DIRECTORY, &path);
		if (status != B_OK)
			return NULL;

		path.Append("mail_daemon");
		path.Append("inbound_protocols");

		BDirectory dir(path.Path());
		entry_ref protocolRef;
		while (dir.GetNextRef(&protocolRef) == B_OK)
		{
			char name[B_FILE_NAME_LENGTH];
			BEntry entry(&protocolRef);
			entry.GetName(name);

			BMenuItem *item;
			BMessage *msg = new BMessage(kProtokollChangedMsg);
			menu->AddItem(item = new BMenuItem(name, msg));
			msg->AddRef("protocol", &protocolRef);

			item->SetMarked(true);
		}
	}

	// make imap default protocol if existing
	BMenuItem* imapItem =  menu->FindItem("IMAP");
	if (imapItem)
		imapItem->SetMarked(true);

	BMenuField *protocolsMenuField = new BMenuField(rect, NULL, NULL, menu);
	protocolsMenuField->ResizeToPreferred();
	return protocolsMenuField;
}


status_t
AutoConfigView::_GetSMTPAddonRef(entry_ref *ref)
{
	directory_which which[] = {
		B_USER_ADDONS_DIRECTORY,
		/*B_COMMON_ADDONS_DIRECTORY,*/
		B_BEOS_ADDONS_DIRECTORY
	};

	for (size_t i = 0; i < sizeof(which) / sizeof(which[0]); i++) {
		BPath path;
		status_t status = find_directory(which[i], &path);
		if (status != B_OK)
			return B_ERROR;

		path.Append("mail_daemon");
		path.Append("outbound_protocols");
		path.Append("SMTP");

		BEntry entry(path.Path());
		if (entry.Exists() && entry.GetRef(ref) == B_OK)
			return B_OK;
	}

	return B_FILE_NOT_FOUND;
}


BString
AutoConfigView::_ExtractLocalPart(const char* email)
{
	const char* at = strrchr(email, '@');
	return BString(email, at - email);
}


void
AutoConfigView::_ProposeUsername()
{
	const char* email = fEmailView->Text();
	provider_info info;
	status_t status = fAutoConfig.GetInfoFromMailAddress(email, &info);
	if (status == B_OK) {
		BString localPart = _ExtractLocalPart(email);
		switch (info.username_pattern) {
			case 0:
				// username is the mail address
				fLoginNameView->SetText(email);
				break;
			case 1:
				// username is the local-part
				fLoginNameView->SetText(localPart.String());
				break;
			case 2:
				// do nothing
				break;
		}
	}
	else {
		fLoginNameView->SetText(email);
	}
}


bool
AutoConfigView::IsValidMailAddress(BString email)
{
	int32 atPos = email.FindFirst("@");
	if (atPos < 0)
		return false;
	BString provider;
	email.CopyInto(provider, atPos + 1, email.Length() - atPos);
	if (provider.FindLast(".") < 0)
		return false;
	return true;
}


// #pragma mark -


ServerSettingsView::ServerSettingsView(BRect rect, const account_info &info)
	:
	BView(rect, NULL,B_FOLLOW_ALL,0),
	fInboundAccount(true),
	fOutboundAccount(true),
	fInboundAuthMenu(NULL),
	fOutboundAuthMenu(NULL),
	fInboundEncrItemStart(NULL),
	fOutboundEncrItemStart(NULL),
	fImageId(-1)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	int32 divider = 120;

	fInboundAccount = true;
	fOutboundAccount = true;

	// inbound
	BRect boxRect = Bounds();
	boxRect.bottom /= 2;
	boxRect.bottom -= 5;

	BBox *box = new BBox(boxRect);
	box->SetLabel(B_TRANSLATE("Incoming"));
	AddChild(box);

	BString serverName;
	if (info.inboundType == IMAP)
		serverName = info.providerInfo.imap_server;
	else
		serverName = info.providerInfo.pop_server;

	fInboundNameView = new BTextControl(BRect(10, 20, rect.Width() - 20, 35),
		"inbound", B_TRANSLATE("Server Name:"), serverName.String(),
		new BMessage(kServerChangedMsg));
	fInboundNameView->SetDivider(divider);

	box->AddChild(fInboundNameView);

	_GetAuthEncrMenu(info.inboundProtocol, &fInboundAuthMenu,
		&fInboundEncryptionMenu);
	if (fInboundAuthMenu != NULL) {
		int authID = info.providerInfo.authentification_pop;
		if (info.inboundType == POP)
			fInboundAuthMenu->Menu()->ItemAt(authID)->SetMarked(true);
		fInboundAuthItemStart = fInboundAuthMenu->Menu()->FindMarked();
		box->AddChild(fInboundAuthMenu);
		fInboundAuthMenu->SetDivider(divider);
		fInboundAuthMenu->MoveTo(10, 50);
	}
	if (fInboundEncryptionMenu) {
		BMenuItem *item = NULL;
		if (info.inboundType == POP) {
			item = fInboundEncryptionMenu->Menu()->ItemAt(
				info.providerInfo.ssl_pop);
			if (item != NULL)
				item->SetMarked(true);
			fInboundEncryptionMenu->MoveTo(10, 80);
		}
		if (info.inboundType == IMAP) {
			item = fInboundEncryptionMenu->Menu()->ItemAt(
				info.providerInfo.ssl_imap);
			if (item != NULL)
				item->SetMarked(true);
			fInboundEncryptionMenu->MoveTo(10, 50);
		}
		fInboundEncrItemStart = fInboundEncryptionMenu->Menu()->FindMarked();
		box->AddChild(fInboundEncryptionMenu);
		fInboundEncryptionMenu->SetDivider(divider);
	}

	if (!fInboundAccount) {
		fInboundNameView->SetEnabled(false);
		if (fInboundAuthMenu)
			fInboundAuthMenu->SetEnabled(false);
	}

	// outbound
	boxRect = Bounds();
	boxRect.top = boxRect.bottom / 2;
	boxRect.top += 5;

	box = new BBox(boxRect);
	box->SetLabel(B_TRANSLATE("Outgoing"));
	AddChild(box);

	serverName = info.providerInfo.smtp_server;
	fOutboundNameView = new BTextControl(BRect(10, 20, rect.Width() - 20, 30),
		"outbound", B_TRANSLATE("Server name:"), serverName.String(),
		new BMessage(kServerChangedMsg));
	fOutboundNameView->SetDivider(divider);

	box->AddChild(fOutboundNameView);

	_GetAuthEncrMenu(info.outboundProtocol, &fOutboundAuthMenu,
		&fOutboundEncryptionMenu);
	if (fOutboundAuthMenu != NULL) {
		BMenuItem *item = fOutboundAuthMenu->Menu()->ItemAt(
			info.providerInfo.authentification_smtp);
		if (item != NULL)
			item->SetMarked(true);
		fOutboundAuthItemStart = item;
		box->AddChild(fOutboundAuthMenu);
		fOutboundAuthMenu->SetDivider(divider);
		fOutboundAuthMenu->MoveTo(10, 50);
	}
	if (fOutboundEncryptionMenu != NULL) {
		BMenuItem *item = fOutboundEncryptionMenu->Menu()->ItemAt(
			info.providerInfo.ssl_smtp);
		if (item != NULL)
			item->SetMarked(true);
		fOutboundEncrItemStart = item;
		box->AddChild(fOutboundEncryptionMenu);
		fOutboundEncryptionMenu->SetDivider(divider);
		fOutboundEncryptionMenu->MoveTo(10, 80);
	}

	if (!fOutboundAccount) {
		fOutboundNameView->SetEnabled(false);
		if (fOutboundAuthMenu)
			fOutboundAuthMenu->SetEnabled(false);
	}

}


ServerSettingsView::~ServerSettingsView()
{
	RemoveChild(fInboundAuthMenu);
	RemoveChild(fInboundEncryptionMenu);
	delete fInboundAuthMenu;
	delete fInboundEncryptionMenu;
	unload_add_on(fImageId);
}


void
ServerSettingsView::GetServerInfo(account_info &info)
{
	if (info.inboundType == IMAP) {
		info.providerInfo.imap_server = fInboundNameView->Text();
		if (fInboundEncryptionMenu != NULL) {
			BMenuItem* item = fInboundEncryptionMenu->Menu()->FindMarked();
			if (item != NULL) {
				info.providerInfo.ssl_imap
					= fInboundEncryptionMenu->Menu()->IndexOf(item);
			}
		}
	} else {
		info.providerInfo.pop_server = fInboundNameView->Text();
		BMenuItem* item = NULL;
		if (fInboundAuthMenu != NULL) {
			item = fInboundAuthMenu->Menu()->FindMarked();
			if (item != NULL) {
				info.providerInfo.authentification_pop
					= fInboundAuthMenu->Menu()->IndexOf(item);
			}
		}
		if (fInboundEncryptionMenu != NULL) {
			item = fInboundEncryptionMenu->Menu()->FindMarked();
			if (item != NULL) {
				info.providerInfo.ssl_pop
					= fInboundEncryptionMenu->Menu()->IndexOf(item);
			}
		}
	}
	info.providerInfo.smtp_server = fOutboundNameView->Text();
	BMenuItem* item = NULL;
	if (fOutboundAuthMenu != NULL) {
		item = fOutboundAuthMenu->Menu()->FindMarked();
		if (item != NULL) {
			info.providerInfo.authentification_smtp
				= fOutboundAuthMenu->Menu()->IndexOf(item);
		}
	}

	if (fOutboundEncryptionMenu != NULL) {
		item = fOutboundEncryptionMenu->Menu()->FindMarked();
		if (item != NULL) {
			info.providerInfo.ssl_smtp
				= fOutboundEncryptionMenu->Menu()->IndexOf(item);
		}
	}
	_DetectMenuChanges();
}


void
ServerSettingsView::_DetectMenuChanges()
{
	bool changed = false;
	if (fInboundAuthMenu != NULL) {
		BMenuItem *item = fInboundAuthMenu->Menu()->FindMarked();
		if (fInboundAuthItemStart != item)
			changed = true;
	}
	if (fInboundEncryptionMenu != NULL) {
		BMenuItem *item = fInboundEncryptionMenu->Menu()->FindMarked();
		if (fInboundEncrItemStart != item)
			changed = true;
	}
	if (fOutboundAuthMenu != NULL) {
		BMenuItem *item = fOutboundAuthMenu->Menu()->FindMarked();
		if (fOutboundAuthItemStart != item)
			changed = true;
	}
	if (fOutboundEncryptionMenu != NULL) {
		BMenuItem *item = fOutboundEncryptionMenu->Menu()->FindMarked();
		if (fOutboundEncrItemStart != item)
			changed = true;
	}
	if (changed) {
		BMessage msg(kServerChangedMsg);
		BMessenger messenger(NULL, Window()->Looper());
		messenger.SendMessage(&msg);
	}
}


void
ServerSettingsView::_GetAuthEncrMenu(entry_ref protocol,
	BMenuField** authField, BMenuField** sslField)
{
	BMailAccountSettings dummySettings;
	BView *view = new BStringView("", "Not here!");//CreateConfigView(protocol, dummySettings.InboundSettings(),
//		dummySettings, fImageId);

	*authField = (BMenuField *)view->FindView("auth_method");
	*sslField = (BMenuField *)view->FindView("flavor");

	view->RemoveChild(*authField);
	view->RemoveChild(*sslField);
	delete view;
}

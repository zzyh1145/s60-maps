/*
 ============================================================================
 Name		: S60MapsAppUi.cpp
 Author	  : artem78
 Copyright   : 
 Description : CS60MapsAppUi implementation
 ============================================================================
 */

// INCLUDE FILES
#include <avkon.hrh>
#include <aknmessagequerydialog.h>
#include <aknnotewrappers.h>
#include <stringloader.h>
#include <s32file.h>
#include <hlplch.h>
#include <apgwgnam.h>

#include <S60Maps_0xED689B88.rsg>

#ifdef _HELP_AVAILABLE_
#include "S60Maps_0xED689B88.hlp.hrh"
#endif
#include "S60Maps.hrh"
#include "S60Maps.pan"
#include "S60MapsApplication.h"
#include "S60MapsAppUi.h"
#include "S60MapsAppView.h"
#include "Defs.h"
#include "GitInfo.h"
#include "FileUtils.h"
//#include <eikprogi.h>
#include <epos_cposlmnearestcriteria.h>
#include <epos_cposlandmarksearch.h>


// ============================ MEMBER FUNCTIONS ===============================


// -----------------------------------------------------------------------------
// CS60MapsAppUi::ConstructL()
// Symbian 2nd phase constructor can leave.
// -----------------------------------------------------------------------------
//
void CS60MapsAppUi::ConstructL()
	{
	// Initialise app UI with standard value.
	BaseConstructL(CAknAppUi::EAknEnableSkin);
	
	iSettings = new (ELeave) CSettings();
	
	// Set several predefined available tiles providers

	// OpenStreetMap standard tile layer
	// https://www.openstreetmap.org/
	iAvailableTileProviders[0] = new (ELeave) TTileProvider(
			_L("osm"), _L("OpenStreetMap"),
			_L8("http://tile.openstreetmap.org/{$z}/{$x}/{$y}.png"),
			0, 19);
	
	// OpenCycleMap
	// https://wiki.openstreetmap.org/wiki/OpenCycleMap
	// https://www.thunderforest.com/maps/opencyclemap/
	_LIT8(KOpenCycleMapUrl, "http://tile.thunderforest.com/cycle/{$z}/{$x}/{$y}.png?apikey=");
	RBuf8 openCycleMapUrl;
	openCycleMapUrl.CreateMaxL(KOpenCycleMapUrl().Length() + KThunderForestApiKey().Length());
	openCycleMapUrl.CleanupClosePushL();
	openCycleMapUrl.Copy(KOpenCycleMapUrl);
	openCycleMapUrl.Append(KThunderForestApiKey);
	iAvailableTileProviders[1] = new (ELeave) TTileProvider(
			_L("opencycle"), _L("OpenCycleMap"),
			openCycleMapUrl,
			0, 22);
	CleanupStack::PopAndDestroy(&openCycleMapUrl);
	
	// Transport Map
	// https://wiki.openstreetmap.org/wiki/Transport_Map
	// https://www.thunderforest.com/maps/transport/
	_LIT8(KTransportMapUrl, "http://tile.thunderforest.com/transport/{$z}/{$x}/{$y}.png?apikey=");
	RBuf8 transportMapUrl;
	transportMapUrl.CreateMaxL(KTransportMapUrl().Length() + KThunderForestApiKey().Length());
	transportMapUrl.CleanupClosePushL();
	transportMapUrl.Copy(KTransportMapUrl);
	transportMapUrl.Append(KThunderForestApiKey);
	iAvailableTileProviders[2] = new (ELeave) TTileProvider(
			_L("transport"), _L("Transport Map"),
			transportMapUrl,
			0, 22);
	CleanupStack::PopAndDestroy(&transportMapUrl);
	
	// Humanitarian Map
	// https://wiki.openstreetmap.org/wiki/Humanitarian_map_style
	// https://www.openstreetmap.org/?layers=H
	iAvailableTileProviders[3] = new (ELeave) TTileProvider(
			_L("humanitarian"), _L("Humanitarian"),
			_L8("http://a.tile.openstreetmap.fr/hot/{$z}/{$x}/{$y}.png"),
			0, 20);
	
	/*// OpenTopoMap
	// https://wiki.openstreetmap.org/wiki/OpenTopoMap
	// https://opentopomap.org/
	// FixMe: Doesn`t work without SSL 
	iAvailableTileProviders[4] = new (ELeave) TTileProvider(
			_L("opentopomap"), _L("OpenTopoMap"),
			_L8("http://tile.opentopomap.org/{$z}/{$x}/{$y}.png"),
			0, 17);*/
	
	iActiveTileProvider = iAvailableTileProviders[0]; // Use first
	
	
	iFileMan = CAsyncFileMan::NewL(CCoeEnv::Static()->FsSession(), this);
	
	// Connect to landmark database
	iLandmarkPartialParameters = CPosLmPartialReadParameters::NewLC();
	CleanupStack::Pop();
	iLandmarkPartialParameters->SetRequestedAttributes(
			CPosLandmark::ELandmarkName | CPosLandmark::EPosition
			/*| CPosLandmark::EIcon*/);
	//User::LeaveIfError(iLandmarkPartialParameters->SetRequestedPositionFields(...));	
	
	iLandmarksDb = CPosLandmarkDatabase::OpenL();
	iLandmarksDb->SetPartialReadParametersL(*iLandmarkPartialParameters);

	// Set initial map position
	TCoordinate position = TCoordinate(iSettings->GetLat(), iSettings->GetLon());
	TZoom zoom = iSettings->GetZoom();	
	
	// Create view object
	iAppView = CS60MapsAppView::NewL(ClientRect(), position, zoom, iActiveTileProvider);
	iAppView->MakeVisible(EFalse); // Will be shown later after settings will be loaded in CS60MapsAppUi::RestoreL
	AddToStackL(iAppView);
	
	// Position requestor
	_LIT(KPosRequestorName, "S60 Maps"); // ToDo: Move to global const
	TRAPD(err, iPosRequestor = CPositionRequestor::NewL(this, KPosRequestorName));
	if (err == KErrNone)
		{
		iPosRequestor->Start(); // Must be started after view created
		}
	else
		{
		HBufC* msg = iEikonEnv->AllocReadResourceLC(R_POSITIONING_DISABLED);
		//CAknWarningNote* note = new (ELeave) CAknWarningNote;
		CAknErrorNote* note = new (ELeave) CAknErrorNote;
		note->ExecuteLD(*msg);
		CleanupStack::PopAndDestroy(msg);
		WARNING(_L("Failed to create position requestor (error: %d), continue without GPS"), err);
		}
	
	// Media keys catching
	iInterfaceSelector = CRemConInterfaceSelector::NewL();
	iCoreTarget = CRemConCoreApiTarget::NewL(*iInterfaceSelector, *this);
	iInterfaceSelector->OpenTargetL();
	
	// Make fullscreen
	SetFullScreenApp(ETrue); // Seems no effect
	StatusPane()->MakeVisible(EFalse); // ToDo: Why if call it before creating
									   // app view panic KERN-EXEC 3 happens?
	//Cba()->MakeVisible(EFalse); // Softkeys not work after this
	iAppView->SetRect(ApplicationRect()); // Need to resize the view to fullscreen
	
	//iCacheResetProgressChecker = CPeriodic::NewL(CActive::EPriorityStandard);
	
	iAppView->SetFollowUser(ETrue);
	}
// -----------------------------------------------------------------------------
// CS60MapsAppUi::CS60MapsAppUi()
// C++ default constructor can NOT contain any code, that might leave.
// -----------------------------------------------------------------------------
//
CS60MapsAppUi::CS60MapsAppUi()
	{
	// No implementation required
	}

// -----------------------------------------------------------------------------
// CS60MapsAppUi::~CS60MapsAppUi()
// Destructor.
// -----------------------------------------------------------------------------
//
CS60MapsAppUi::~CS60MapsAppUi()
	{
	//delete iCoreTarget;
	/* Panic KERN-EXEC 3 - Seems that there are no need to manually destroy core target,
	 because interface selector brings ownership and will delete target by itself. */
	
	/*iCacheResetProgressChecker->Cancel();
	delete iCacheResetProgressChecker;*/
	
	//delete iCacheClearingWaitDialog;
	
	delete iInterfaceSelector;
	
	delete iPosRequestor;
	
	if (iAppView)
		{
		//if (IsControlOnStack(iAppView))
			RemoveFromStack(iAppView);
		delete iAppView;
		iAppView = NULL;
		}
	
	delete iLandmarksDb;
	delete iLandmarkPartialParameters;
	
	delete iFileMan;
	
	//delete iAvailableTileProviders;
	iAvailableTileProviders.DeleteAll();
	
	delete iSettings;
	}

// -----------------------------------------------------------------------------
// CS60MapsAppUi::HandleCommandL()
// Takes care of command handling.
// -----------------------------------------------------------------------------
//
void CS60MapsAppUi::HandleCommandL(TInt aCommand)
	{
	switch (aCommand)
		{
		case EEikCmdExit:
		case EAknSoftkeyExit:
			HandleExitL();
			break;
			
		case EFindMe:
			HandleFindMeL();
			break;
			
		case ESetOsmStandardTileProvider:
		case ESetOsmCyclesTileProvider:
		case ESetOsmHumanitarianTileProvider:
		case ESetOsmTransportTileProvider:
		//case ESetOpenTopoMapTileProvider:
			HandleTileProviderChangeL(aCommand - ESetTileProviderBase);
			break;
			
		case ETilesCacheStats:
			HandleTilesCacheStatsL();
			break;
			
		case EResetTilesCache:
			HandleTilesCacheResetL();
			break;
			
#ifdef _HELP_AVAILABLE_
		case EHelp:
			HandleHelpL();
			break;
#endif
			
		case EAbout:
			HandleAboutL();
			break;
			
		case EToggleLandmarksVisibility:
			HandleToggleLandmarksVisibility();
			break;
			
		case ECreateLandmark:
			HandleCreateLandmarkL();
			break;
			
		case ERenameLandmark:
			HandleRenameLandmarkL();
			break;
			
		case ECreateOrRenameLandmark:
			HandleCreateOrRenameLandmarkL();
			break;
			
		case EDeleteLandmark:
			HandleDeleteLandmarkL();
			break;
			
		case EGotoLandmark:
			HandleGotoLandmarkL();
			break;
			
		case EGotoCoordinate:
			HandleGotoCoordinateL();
			break;
			
		default:
			Panic(ES60MapsUi);
			break;
		}
	}

// -----------------------------------------------------------------------------
//  Called by the framework when the application status pane
//  size is changed.  Passes the new client rectangle to the
//  AppView
// -----------------------------------------------------------------------------
//
void CS60MapsAppUi::HandleStatusPaneSizeChange()
	{
	TCoordinate coord = iAppView->GetCenterCoordinate();
	if (iAppView->IsSoftkeysShown())
		{
		iAppView->SetRect(ClientRect());
		}
	else
		{
		iAppView->SetRect(ApplicationRect());
		}
	iAppView->Move(coord);
	}

CArrayFix<TCoeHelpContext>* CS60MapsAppUi::HelpContextL() const
	{
#warning "Please see comment about help and UID3..."
	// Note: Help will not work if the application uid3 is not in the
	// protected range.  The default uid3 range for projects created
	// from this template (0xE0000000 - 0xEFFFFFFF) are not in the protected range so that they
	// can be self signed and installed on the device during testing.
	// Once you get your official uid3 from Symbian Ltd. and find/replace
	// all occurrences of uid3 in your project, the context help will
	// work. Alternatively, a patch now exists for the versions of 
	// HTML help compiler in SDKs and can be found here along with an FAQ:
	// http://www3.symbian.com/faq.nsf/AllByDate/E9DF3257FD565A658025733900805EA2?OpenDocument
#ifdef _HELP_AVAILABLE_
	CArrayFixFlat<TCoeHelpContext>* array = new(ELeave)CArrayFixFlat<TCoeHelpContext>(1);
	CleanupStack::PushL(array);
	array->AppendL(TCoeHelpContext(KUidS60MapsApp, KGeneral_Information));
	CleanupStack::Pop(array);
	return array;
#else
	return NULL;
#endif
	}

void CS60MapsAppUi::DynInitMenuPaneL(TInt aMenuID, CEikMenuPane* aMenuPane)
	{
	if (aMenuID == R_MENU)
		{
		//aMenuPane->SetItemButtonState(EFindMe,
		//		iAppView->IsFollowingUser() ? EEikMenuItemSymbolOn : EEikMenuItemSymbolIndeterminate
		//);
		aMenuPane->SetItemDimmed(EFindMe, !iPosRequestor || iAppView->IsFollowingUser());
		}
	else if (aMenuID == R_SUBMENU_GOTO)
		{
		CPosLmItemIterator* landmarkIterator = iLandmarksDb->LandmarkIteratorL();
		if (!(landmarkIterator && landmarkIterator->NumOfItemsL() > 0))
			aMenuPane->SetItemDimmed(EGotoLandmark, ETrue);
		delete landmarkIterator;
		}
	else if (aMenuID == R_SUBMENU_TILE_PROVIDERS)
		{
		// Fill list of available tiles services in menu
		
		for (TInt idx = 0; idx < iAvailableTileProviders.Count(); idx++)
			{
			TInt commandId = ESetTileProviderBase + idx;
			
			CEikMenuPaneItem::SData menuItem;
			menuItem.iCommandId = commandId;
			menuItem.iCascadeId = 0;
			menuItem.iFlags = EEikMenuItemCheckBox;
			menuItem.iText.Copy(iAvailableTileProviders[idx]->iTitle);
			menuItem.iExtraText.Zero();
			aMenuPane->AddMenuItemL(menuItem);
			aMenuPane->SetItemButtonState(commandId,
					/*commandId == selectedTileProviderCommId*/
					iAvailableTileProviders[idx] == iActiveTileProvider?
							EEikMenuItemSymbolOn : EEikMenuItemSymbolIndeterminate);				
			}
		}
	else if (aMenuID == R_SUBMENU_LANDMARKS)
		{
		aMenuPane->SetItemButtonState(EToggleLandmarksVisibility,
				iSettings->GetLandmarksVisibility() ? EEikMenuItemSymbolOn : EEikMenuItemSymbolIndeterminate
		);
		CPosLandmark* nearestLandmark = GetNearestLandmarkAroundTheCenterL();
		TBool isDisplayEditOrDeleteLandmark = iSettings->GetLandmarksVisibility() && nearestLandmark;
		delete nearestLandmark;
		aMenuPane->SetItemDimmed(ERenameLandmark, !isDisplayEditOrDeleteLandmark);
		aMenuPane->SetItemDimmed(EDeleteLandmark, !isDisplayEditOrDeleteLandmark);
		}
	/*else
		{
		AppUi()->DynInitMenuPaneL(aMenuID, aMenuPane);
		}*/
	}

TStreamId CS60MapsAppUi::StoreL(CStreamStore& aStore) const
	{
	RStoreWriteStream stream;
	TStreamId id = stream.CreateLC(aStore);
	stream << *this;
	stream.CommitL() ;
	CleanupStack::PopAndDestroy(&stream);
	return id;
	}

void CS60MapsAppUi::RestoreL(const CStreamStore& aStore,
		TStreamId aStreamId)
	{
	RStoreReadStream stream;
	stream.OpenLC(aStore, aStreamId);
	stream >> *this;
	CleanupStack::PopAndDestroy(&stream);
	
	DEBUG(_L("Settings restored"));
	
	// Show fully constructed view
	iAppView->MakeVisible(ETrue);
	}

void CS60MapsAppUi::ExternalizeL(RWriteStream& aStream) const
	{
	// Update settings
	TCoordinate coord = iAppView->GetCenterCoordinate();
	iSettings->SetLat(coord.Latitude());
	iSettings->SetLon(coord.Longitude());
	iSettings->SetZoom(iAppView->GetZoom());
	iSettings->SetTileProviderId(iActiveTileProvider->iId);	
	
	// And save
	aStream << *iSettings;
	}

void CS60MapsAppUi::InternalizeL(RReadStream& aStream)
	{
	TRAP_IGNORE(aStream >> *iSettings);
	iAppView->Move(iSettings->GetLat(), iSettings->GetLon(), iSettings->GetZoom());
	
	TTileProviderId tileProviderId(iSettings->GetTileProviderId());
	TBool isFound = EFalse;
	for (TInt idx = 0; idx < iAvailableTileProviders.Count(); idx++)
		{
		if (tileProviderId == iAvailableTileProviders[idx]->iId)
			{
			iActiveTileProvider = iAvailableTileProviders[idx];
			iAppView->SetTileProviderL(iAvailableTileProviders[idx]);
			isFound = ETrue;
			break;
			}
		}
	
	if (!isFound)
		{ // Set default
		iActiveTileProvider = iAvailableTileProviders[0];
		iAppView->SetTileProviderL(iAvailableTileProviders[0]);
		}
	}

MFileManObserver::TControl CS60MapsAppUi::NotifyFileManStarted()
	{
	return MFileManObserver::EContinue;
	}

MFileManObserver::TControl CS60MapsAppUi::NotifyFileManOperation()
	{
	return MFileManObserver::EContinue;
	}

MFileManObserver::TControl CS60MapsAppUi::NotifyFileManEnded()
	{
	return MFileManObserver::EContinue;
	}

void CS60MapsAppUi::ClearTilesCacheL()
	{
	TFileName cacheDir;
	static_cast<CS60MapsApplication *>(Application())->CacheDir(cacheDir);
	
	INFO(_L("Start cleaning of cache directory"));
	iFileMan->Delete(cacheDir, CFileMan::ERecurse);
	
	/*// Prepare and show progress dialog
	iCacheResetProgressDialog = new (ELeave) CAknProgressDialog(
			REINTERPRET_CAST(CEikDialog**, &iCacheResetProgressDialog)
	);
	iCacheResetProgressDialog->ExecuteLD(R_PROGRESS_DIALOG);
	const KInterval = KSecond / 5;
	TCallBack callback(UpdateTilesClearingProgress, this);*/
	//iCacheResetProgressChecker->Start(0/*KInterval*/, KInterval, callback);
	
	// Prepare and show wait dialog
	iCacheClearingWaitDialog = new (ELeave) CAknWaitDialog(
			REINTERPRET_CAST(CEikDialog**, &iCacheClearingWaitDialog)
	);
	iCacheClearingWaitDialog->SetCallback(this);
	iCacheClearingWaitDialog->ExecuteLD(R_WAIT_DIALOG);
	}

void CS60MapsAppUi::OnFileManFinished(TInt aStatus)
	{
	INFO(_L("FileMan operation ended with code=%d"), aStatus);
	
	//iCacheResetProgressDialog->ProcessFinishedL();
	if (iCacheClearingWaitDialog)
		iCacheClearingWaitDialog->ProcessFinishedL();
	
	if (aStatus == KErrNone)
		{
		// Show "Done" message
		HBufC* msg = iEikonEnv->AllocReadResourceLC(R_DONE);
		CAknInformationNote* note = new (ELeave) CAknInformationNote;
		note->ExecuteLD(*msg);
		CleanupStack::PopAndDestroy(msg);
		}
	}

/*TInt CS60MapsAppUi::UpdateTilesClearingProgress(TAny* aSelfPtr)
	{
	CS60MapsAppUi* self = static_cast<CS60MapsAppUi*>(aSelfPtr);
	CEikProgressInfo* progressInfo = self->iCacheResetProgressDialog->GetProgressInfoL();
	progressInfo->SetFinalValue(self->iFileMan->TotalFiles());
	progressInfo->SetAndDraw(self->iFileMan->ProcessedFiles());
	DEBUG(_L("Progress: %d/%d"), self->iFileMan->ProcessedFiles(), self->iFileMan->TotalFiles());
	
	return ETrue;
	}*/

void CS60MapsAppUi::OnPositionUpdated()
	{
	__ASSERT_DEBUG(iPosRequestor != NULL, Panic(ES60MapsPosRequestorIsNull));
	
	const TPositionInfo* posInfo = iPosRequestor->LastKnownPositionInfo();
	TPosition pos;
	posInfo->GetPosition(pos);
	TCoordinateEx coord = pos;
	coord.SetCourse(KNaN);
	if (posInfo->PositionClassType() & EPositionCourseInfoClass)
		{
		const TPositionCourseInfo* courseInfo =
				static_cast<const TPositionCourseInfo*>(posInfo);
		TCourse course;
		courseInfo->GetCourse(course);
		
		coord.SetCourse(course.Heading());
		}
	coord.SetHorAccuracy(pos.HorizontalAccuracy());
	iAppView->SetUserPosition(coord);
	}

void CS60MapsAppUi::OnPositionPartialUpdated()
	{
	
	}

void CS60MapsAppUi::OnPositionRestored()
	{
	}

void CS60MapsAppUi::OnPositionLost()
	{
	iAppView->HideUserPosition();
	}

void CS60MapsAppUi::OnPositionError(TInt /*aErrCode*/)
	{
	
	}

void CS60MapsAppUi::MrccatoCommand(TRemConCoreApiOperationId aOperationId,
		TRemConCoreApiButtonAction aButtonAct)
	{
	//TRequestStatus status;
	
	if (aButtonAct == ERemConCoreApiButtonClick)
		{
		switch (aOperationId)
			{
			case ERemConCoreApiVolumeUp:
				{
				//DEBUG(_L("VolumeUp pressed\n"));
				iAppView->ZoomIn();
				
				/*iCoreTarget->VolumeUpResponse(status, KErrNone);
				User::WaitForRequest(status);*/
				}
			break;
			
			case ERemConCoreApiVolumeDown:
				{
				//DEBUG(_L("VolumeDown pressed\n"));
				iAppView->ZoomOut();
				
				/*iCoreTarget->VolumeDownResponse(status, KErrNone);
				User::WaitForRequest(status);*/
				}
			break;
			
			default:
			break;
			}
		}
	}

void CS60MapsAppUi::DialogDismissedL(TInt aButtonId)
	{
	if (aButtonId != EAknSoftkeyCancel)
		return;
	
	iFileMan->Cancel();
	
	INFO(_L("Clearing cache operation cancelled"));
	}

void CS60MapsAppUi::HandleExitL()
	{
	RWsSession& session = CEikonEnv::Static()->WsSession();
	TInt WgId = session.GetFocusWindowGroup();
	CApaWindowGroupName* Wgn = CApaWindowGroupName::NewL(session, WgId);
	TUid forgroundApp = Wgn->AppUid();
	delete Wgn;
	const TUid KAppUid = {_UID3};
	//If application is in background Symbian OS will show its own quit confirmation.
	if(forgroundApp == KAppUid)
		{
		CAknQueryDialog* dlg = CAknQueryDialog::NewL();
		dlg->PrepareLC(R_CONFIRM_DIALOG);
		/*HBufC* title = iEikonEnv->AllocReadResourceLC(R_CONFIRM_EXIT_DIALOG_TITLE);
		dlg->SetHeaderTextL(*title);
		CleanupStack::PopAndDestroy(); //title*/
		HBufC* msg = iEikonEnv->AllocReadResourceLC(R_CONFIRM_EXIT_DIALOG_TEXT);
		dlg->SetPromptL(*msg);
		CleanupStack::PopAndDestroy(); //msg
		TInt res = dlg->RunLD();
		if (res != EAknSoftkeyYes)
			{
			return;
			}
		}
	
	// Send window to background to increase visible speed of shutdown
	SendAppToBackground();
	
	// Save settings and exit
	SaveL();
	Exit();
	}

void CS60MapsAppUi::HandleFindMeL()
	{
	iAppView->SetFollowUser(ETrue);
	}

void CS60MapsAppUi::HandleTileProviderChangeL(TInt aTileProviderIdx)
	{
	iActiveTileProvider = iAvailableTileProviders[aTileProviderIdx];
	static_cast<CS60MapsAppView*>(iAppView)->SetTileProviderL(iActiveTileProvider);
	}

void CS60MapsAppUi::HandleTilesCacheStatsL()
	{
	CS60MapsApplication* app = static_cast<CS60MapsApplication *>(Application());
	RFs fs = iEikonEnv->FsSession();
	
	// Prepare information text
	RBuf msg;
	msg.CreateL(2048);
	msg.CleanupClosePushL();
	
	TInt filesTotal = 0, bytesTotal = 0;
	
	TFileName baseCacheDir;
	app->CacheDir(baseCacheDir);
	
	CDir* cacheSubDirs = NULL;
	TInt r = fs.GetDir(baseCacheDir, KEntryAttDir, ESortByName, cacheSubDirs);
	if (r == KErrNone && cacheSubDirs != NULL)
		{
		for (TInt i = 0; i < cacheSubDirs->Count(); i++)
			{
			const TEntry &cacheSubDir = (*cacheSubDirs)[i];
			
			// Seems that KEntryAttDir doesn`t work
			if (!cacheSubDir.IsDir())
				continue;
			
			TDirStats dirStats;
			RBuf subDirFullPath;
			subDirFullPath.Create(KMaxFileName);
			subDirFullPath.Copy(baseCacheDir);
			TParsePtr parser(subDirFullPath);
			parser.AddDir(cacheSubDir.iName);
			r = FileUtils::DirectoryStats(fs, parser.FullName(), dirStats);
			subDirFullPath.Close();
			if (r != KErrNone)
				{ // Something went wrong
				dirStats.iFilesCount = 0;
				dirStats.iSize = 0;
				}
			
			filesTotal += dirStats.iFilesCount;
			bytesTotal += dirStats.iSize;
			
			TBuf<16> sizeBuff;
			FileUtils::FileSizeToReadableString(dirStats.iSize, sizeBuff);
			
			TPtrC itemName(cacheSubDir.iName);
			for (TInt providerIdx = 0; providerIdx < iAvailableTileProviders.Count(); providerIdx++)
				{
				if (iAvailableTileProviders[providerIdx]->iId == cacheSubDir.iName)
					{
					itemName.Set(iAvailableTileProviders[providerIdx]->iTitle);
					break;
					}
				}
			
			TBuf<64> buf;
			iEikonEnv->Format128(buf, R_STATS_LINE, &itemName, dirStats.iFilesCount, &sizeBuff);
			msg.Append(buf);
			}
		
		delete cacheSubDirs;
		}
	
	msg.Append(_L("------------\n"));
	TBuf<16> totalSizeBuff;
	FileUtils::FileSizeToReadableString(bytesTotal, totalSizeBuff);
	TBuf<64> buf;
	iEikonEnv->Format128(buf, R_STATS_TOTAL, filesTotal, &totalSizeBuff);
	msg.Append(buf);
	
	
	
	// Show information
	CAknMessageQueryDialog* dlg = new (ELeave) CAknMessageQueryDialog();
	dlg->PrepareLC(R_QUERY_DIALOG);
	HBufC* title = iEikonEnv->AllocReadResourceLC(R_MAP_CACHE_STATS_DIALOG_TITLE);
	dlg->QueryHeading()->SetTextL(*title);
	CleanupStack::PopAndDestroy(title);
	dlg->SetMessageTextL(msg);
	//CleanupStack::PopAndDestroy(&msg);
	dlg->RunLD();
	
	CleanupStack::PopAndDestroy(&msg);
	//CleanupStack::PopAndDestroy(/*3*/2, &msg);
	}

void CS60MapsAppUi::HandleTilesCacheResetL()
	{
	CAknQueryDialog* dlg = CAknQueryDialog::NewL();
	dlg->PrepareLC(R_CONFIRM_DIALOG);
	/*HBufC* title = iEikonEnv->AllocReadResourceLC(R_CONFIRM_RESET_TILES_CACHE_DIALOG_TITLE);
	dlg->SetHeaderTextL(*title);
	CleanupStack::PopAndDestroy(); //title*/
	HBufC* msg = iEikonEnv->AllocReadResourceLC(R_CONFIRM_RESET_TILES_CACHE_DIALOG_TEXT);
	dlg->SetPromptL(*msg);
	CleanupStack::PopAndDestroy(); //msg
	TInt res = dlg->RunLD();
	if (res == EAknSoftkeyYes)
		{
		ClearTilesCacheL();
		}
	}

#ifdef _HELP_AVAILABLE_
void CS60MapsAppUi::HandleHelpL()
	{
	CArrayFix<TCoeHelpContext>* buf = CCoeAppUi::AppHelpContextL();
	HlpLauncher::LaunchHelpApplicationL(iEikonEnv->WsSession(), buf);
	}
#endif

void CS60MapsAppUi::HandleAboutL()
	{
	_LIT(KAuthor,	"artem78 (megabyte1024@ya.ru)");
	_LIT(KWebSite,	"https://github.com/artem78/s60-maps");
	_LIT(KThanksTo,	"baranovskiykonstantin, Symbian9, Men770, fizolas");
	
	CAknMessageQueryDialog* dlg = new (ELeave) CAknMessageQueryDialog();
	dlg->PrepareLC(R_QUERY_DIALOG);
	HBufC* title = iEikonEnv->AllocReadResourceLC(R_ABOUT_DIALOG_TITLE);
	dlg->QueryHeading()->SetTextL(*title);
	CleanupStack::PopAndDestroy(); //title
	
	RBuf msg;
	msg.CreateL(512);
	msg.CleanupClosePushL();
	TBuf<32> version;
	version.Copy(KProgramVersion.Name());
#ifdef _DEBUG
	_LIT(KDebug, "DEBUG");
	version.Append(' ');
	version.Append(KDebug);
#endif
	iEikonEnv->Format128/*256*/(msg, R_ABOUT_DIALOG_TEXT, &version,
			&KGITBranch, &KGITCommit, &KAuthor, &KWebSite, &KThanksTo);
	dlg->SetMessageTextL(msg);
	CleanupStack::PopAndDestroy(&msg);
	dlg->RunLD();
	}

void CS60MapsAppUi::HandleToggleLandmarksVisibility()
	{
	iSettings->SetLandmarksVisibility(!iSettings->GetLandmarksVisibility());
	}

void CS60MapsAppUi::HandleCreateLandmarkL()
	{
	_LIT(KDefaultLandmarkName, "Landmark");
	_LIT(KLandmarkCategoryName, "S60Maps");
	
	RBuf landmarkName;
	landmarkName.CreateL(KPosLmMaxTextFieldLength);
	CleanupClosePushL(landmarkName);
	landmarkName.Copy(KDefaultLandmarkName);
	
	// Create landmark
	CPosLandmark* newLandmark = CPosLandmark::NewLC();
	TLocality pos = TLocality(iAppView->GetCenterCoordinate(), KNaN, KNaN);
	newLandmark->SetPositionL(pos);
	// Ask landmark name
	HBufC* dlgTitle = iEikonEnv->AllocReadResourceLC(R_INPUT_NAME);
	CAknTextQueryDialog* dlg = new (ELeave) CAknTextQueryDialog(landmarkName);
	if (dlg->ExecuteLD(R_LANDMARK_NAME_INPUT_QUERY, *dlgTitle) == EAknSoftkeyOk)
		{
		// Set landmark name
		newLandmark->SetLandmarkNameL(landmarkName);
		
		// Set landmark category
		CPosLmCategoryManager* catMgr = CPosLmCategoryManager::NewL(*iLandmarksDb);
		CleanupStack::PushL(catMgr);
		TPosLmItemId catId = catMgr->GetCategoryL(KLandmarkCategoryName);
		if (catId == KPosLmNullItemId)
			{ // If category didn`t found, create it
			CPosLandmarkCategory* category = CPosLandmarkCategory::NewLC();
			category->SetCategoryNameL(KLandmarkCategoryName);
			catId = catMgr->AddCategoryL(*category);	
			CleanupStack::PopAndDestroy(category);
			}
		newLandmark->AddCategoryL(catId);
		
		// Save landmark to DB
		iLandmarksDb->AddLandmarkL(*newLandmark);
		iAppView->DrawDeferred();
		
		CleanupStack::PopAndDestroy(catMgr);
		}

	CleanupStack::PopAndDestroy(3, &landmarkName);
	}

void CS60MapsAppUi::HandleRenameLandmarkL()
	{
	if (!iSettings->GetLandmarksVisibility())
		return;
	
	CPosLandmark* landmark = GetNearestLandmarkAroundTheCenterL(EFalse);
	if (!landmark)
		return; // Nothing to do
	CleanupStack::PushL(landmark);
	
	RBuf landmarkName;
	landmarkName.CreateL(KPosLmMaxTextFieldLength);
	CleanupClosePushL(landmarkName);
	
	TPtrC oldLandmarkName;
	landmark->GetLandmarkName(oldLandmarkName);
	landmarkName.Copy(oldLandmarkName);
	
	// Ask landmark new name
	HBufC* dlgTitle = iEikonEnv->AllocReadResourceLC(R_INPUT_NAME);
	CAknTextQueryDialog* dlg = new (ELeave) CAknTextQueryDialog(landmarkName);
	if (dlg->ExecuteLD(R_LANDMARK_NAME_INPUT_QUERY, *dlgTitle) == EAknSoftkeyOk)
		{
		// Update landmark in DB	
		landmark->SetLandmarkNameL(landmarkName);
		iLandmarksDb->UpdateLandmarkL(*landmark);
		iAppView->DrawDeferred();
		}
	
	CleanupStack::PopAndDestroy(3, landmark);
	}

void CS60MapsAppUi::HandleCreateOrRenameLandmarkL()
	{
	CPosLandmark* landmark = GetNearestLandmarkAroundTheCenterL(ETrue);
	if (landmark /*&& iSettings->GetLandmarksVisibility()*/)
		{
		delete landmark;
		HandleRenameLandmarkL();
		}
	else
		{
		HandleCreateLandmarkL();
		}
	}

void CS60MapsAppUi::HandleDeleteLandmarkL()
	{
	if (!iSettings->GetLandmarksVisibility())
		return;
	
	CPosLandmark* landmark = GetNearestLandmarkAroundTheCenterL(ETrue);
	if (!landmark)
		return;
	CleanupStack::PushL(landmark);
	
	// Ask for confirmation
	CAknQueryDialog* dlg = CAknQueryDialog::NewL();
	dlg->PrepareLC(R_CONFIRM_DIALOG);
	TPtrC landmarkName;
	landmark->GetLandmarkName(landmarkName);
	TBuf<128/*256*/> msg;
	iEikonEnv->Format128(msg, R_CONFIRM_LANDMARK_DELETION, &landmarkName);
	dlg->SetPromptL(msg);
	if (dlg->RunLD() == EAknSoftkeyYes)
		{
		// Remove landmark from DB
		iLandmarksDb->RemoveLandmarkL(landmark->LandmarkId());
		iAppView->DrawDeferred();
		}
	
	CleanupStack::PopAndDestroy(landmark);
	}

void CS60MapsAppUi::HandleGotoLandmarkL()
	{
	const TInt KGranularity = 50;
	CDesCArraySeg* lmNameArray = new (ELeave) CDesCArraySeg(KGranularity);
	CleanupStack::PushL(lmNameArray);
	CArrayFixFlat<TPosLmItemId>* lmIdArray = new (ELeave) CArrayFixFlat<TPosLmItemId>(KGranularity);
	CleanupStack::PushL(lmIdArray);
	CPosLmItemIterator* lmIterator = iLandmarksDb->LandmarkIteratorL();
	CleanupStack::PushL(lmIterator);

	TPosLmItemId lmId;
	lmId = lmIterator->NextL();
	while (lmId != KPosLmNullItemId)
		{
		CPosLandmark* lm = iLandmarksDb->ReadPartialLandmarkLC(lmId);
		//if (lm->IsPositionFieldAvailable())
		TLocality pos;
		if (lm->GetPosition(pos) == KErrNone && !Math::IsNaN(pos.Latitude())
				&& !Math::IsNaN(pos.Longitude()))
			{ // Process landmarks only with position
			TPtrC lmName;
			lm->GetLandmarkName(lmName);
			lmNameArray->AppendL(lmName);
			lmIdArray->AppendL(lmId);
			}
		CleanupStack::PopAndDestroy(lm);
		lmId = lmIterator->NextL();
		}

	TInt chosenItem;
	CAknListQueryDialog* dlg = new(ELeave) CAknListQueryDialog(&chosenItem);
	dlg->PrepareLC(R_LANDMARKS_QUERY_DIALOG);
	dlg->SetItemTextArray(lmNameArray);
	dlg->SetOwnershipType(ELbmDoesNotOwnItemArray);
	TInt answer = dlg->RunLD();
	if (EAknSoftkeyOk == answer)
		{
		CPosLandmark* lm = iLandmarksDb->ReadLandmarkLC(lmIdArray->At(chosenItem));
		TLocality pos;
		lm->GetPosition(pos);
		iAppView->MoveAndZoomIn(pos);
		CleanupStack::PopAndDestroy(lm);
		}

	CleanupStack::PopAndDestroy(3, lmNameArray);
	}

void CS60MapsAppUi::HandleGotoCoordinateL()
	{
	TCoordinate coord = iAppView->GetCenterCoordinate();
	TPosition pos;
	pos.SetCoordinate(coord.Latitude(), coord.Longitude());
	CAknMultiLineDataQueryDialog* dlg=CAknMultiLineDataQueryDialog::NewL(pos);
	if(dlg->ExecuteLD(R_LOCATION_QUERY_DIALOG) == EAknSoftkeyOk)
		{
		coord.SetCoordinate(pos.Latitude(), pos.Longitude());
		iAppView->MoveAndZoomIn(coord);
		}
	}

void CS60MapsAppUi::SendAppToBackground()
	{
	TApaTask task(iEikonEnv->WsSession());
	task.SetWgId(CEikonEnv::Static()->RootWin().Identifier());
	task.SendToBackground();
	}

CPosLandmark* CS60MapsAppUi::GetNearestLandmarkL(const TCoordinate &aCoord,
		TBool aPartial, TReal32 aMaxDistance)
	{
	CPosLandmark* landmark = NULL; // Returned value
	
	DEBUG(_L("Start nearest landmark queing (max distance = %f m.)"), aMaxDistance);
	
	CPosLandmarkSearch* landmarkSearch = CPosLandmarkSearch::NewL(*iLandmarksDb);
	CleanupStack::PushL(landmarkSearch);
	landmarkSearch->SetMaxNumOfMatches(1);
	CPosLmNearestCriteria* nearestCriteria = CPosLmNearestCriteria::NewLC(aCoord);
	nearestCriteria->SetMaxDistance(aMaxDistance);
	CPosLmOperation* landmarkOp = landmarkSearch->StartLandmarkSearchL(*nearestCriteria, EFalse);
	ExecuteAndDeleteLD(landmarkOp);
	if (landmarkSearch->NumOfMatches())
		{		
		CPosLmItemIterator* landmarkIter = landmarkSearch->MatchIteratorL();
		CleanupStack::PushL(landmarkIter);
		
		landmarkIter->Reset();
		TPosLmItemId landmarkId = landmarkIter->NextL();
		if (landmarkId == KPosLmNullItemId)
			User::Leave(KErrNotFound);
		
		if (aPartial)
			landmark = iLandmarksDb->ReadPartialLandmarkLC(landmarkId);
		else
			landmark = iLandmarksDb->ReadLandmarkLC(landmarkId);
		
		CleanupStack::Pop(); // landmark
		
		CleanupStack::PopAndDestroy(landmarkIter);		
		}
		
	CleanupStack::PopAndDestroy(2, landmarkSearch);
	
	DEBUG(_L("End nearest landmark queing"));
	
	if (!landmark)
		{
		DEBUG(_L("Nothing found"));
		}
	
	return landmark;
	}

CPosLandmark* CS60MapsAppUi::GetNearestLandmarkAroundTheCenterL(TBool aPartial)
	{
	const TInt KMaxDistanceInPixels = 25;
	
	TCoordinate center = iAppView->GetCenterCoordinate();
	TReal32 maxDistance /* in meters */, unused;
	MapMath::PixelsToMeters(center.Latitude(), iAppView->GetZoom(),
			KMaxDistanceInPixels, maxDistance, unused);
	return GetNearestLandmarkL(center, aPartial, maxDistance);
	}

// End of File

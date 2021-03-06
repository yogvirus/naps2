// NAPS2.WIA.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <queue>
#include <functional>

class CWiaTransferCallback1 : public IWiaDataCallback
{
public: // Constructors, destructor
	CWiaTransferCallback1(bool __stdcall statusCallback(LONG, LONG, ULONG64, HRESULT, IStream*)) : m_cRef(1), m_statusCallback(statusCallback) {}
	~CWiaTransferCallback1() {};

	HRESULT CALLBACK QueryInterface(REFIID riid, void **ppvObject) override
	{
		// Validate arguments
		if (NULL == ppvObject)
		{
			return E_INVALIDARG;
		}

		// Return the appropropriate interface
		if (IsEqualIID(riid, IID_IUnknown))
		{
			*ppvObject = static_cast<IUnknown*>(this);
		}
		else if (IsEqualIID(riid, IID_IWiaDataCallback))
		{
			*ppvObject = static_cast<IWiaDataCallback*>(this);
		}
		else
		{
			*ppvObject = NULL;
			return (E_NOINTERFACE);
		}

		// Increment the reference count before we return the interface
		reinterpret_cast<IUnknown*>(*ppvObject)->AddRef();
		return S_OK;
	}

	ULONG CALLBACK AddRef() override
	{
		return InterlockedIncrement((long*)&m_cRef);
	}

	ULONG CALLBACK Release() override
	{
		LONG cRef = InterlockedDecrement((long*)&m_cRef);
		if (0 == cRef)
		{
			delete this;
		}
		return cRef;
	}

	HRESULT __stdcall BandedDataCallback(LONG lMessage, LONG lStatus, LONG lPercentComplete, LONG lOffset, LONG lLength,
		LONG lReserved, LONG lResLength, BYTE* pbBuffer)
	{
		HRESULT hr = S_OK;
		IStream *stream = NULL;

		switch(lMessage)
		{
		case IT_MSG_DATA_HEADER:
			break;
		case IT_MSG_DATA:
			if (m_stream == NULL)
			{
				m_stream = SHCreateMemStream(nullptr, 0);
				BYTE empty_header[] = { 0x42,0x4D,0,0,0,0,0,0,0,0,0,0,0,0 };
				m_stream->Write(empty_header, 14, NULL);
			}
			m_stream->Write(pbBuffer, lLength, NULL);
			break;
		case IT_MSG_STATUS:
			break;
		case IT_MSG_NEW_PAGE:
			stream = m_stream;
			m_stream = NULL;
			break;
		case IT_MSG_TERMINATION:
			stream = m_stream;
			break;
		default:
			break;
		}

		if (stream != NULL)
		{
			LARGE_INTEGER i = { 0 };
			i.QuadPart = 14;
			stream->Seek(i, STREAM_SEEK_SET, NULL);
			int header_bytes;
			stream->Read(&header_bytes, 4, NULL);
			header_bytes += 14;
			i.QuadPart = 2;
			stream->Seek(i, STREAM_SEEK_SET, NULL);
			STATSTG stat;
			stream->Stat(&stat, 1);
			stream->Write(&stat.cbSize, 4, NULL);
			i.QuadPart = 10;
			stream->Seek(i, STREAM_SEEK_SET, NULL);
			stream->Write(&header_bytes, 4, NULL);
			i.QuadPart = 0;
			stream->Seek(i, STREAM_SEEK_SET, NULL);
			m_statusCallback(
				2,
				lPercentComplete,
				lOffset + lLength,
				lStatus,
				stream);
		}

		if (!m_statusCallback(
			lMessage == IT_MSG_TERMINATION ? 3 : 1,
			lPercentComplete,
			lOffset + lLength,
			lStatus,
			NULL))
		{
			hr = S_FALSE;
		}

		if (stream)
		{
			stream->Release();
		}

		return hr;
	}

private:
	bool cancel;
	LONG m_cRef;
	IStream* m_stream;
	std::function<bool(LONG, LONG, ULONG64, HRESULT, IStream*)> m_statusCallback;
};

class CWiaTransferCallback2 : public IWiaTransferCallback
{
public: // Constructors, destructor
	CWiaTransferCallback2(bool __stdcall statusCallback(LONG, LONG, ULONG64, HRESULT, IStream*)) : m_cRef(1), m_statusCallback(statusCallback) {};
	~CWiaTransferCallback2() {};

public: // IWiaTransferCallback
	HRESULT __stdcall TransferCallback(
		LONG                lFlags,
		WiaTransferParams   *pWiaTransferParams) override
	{
		HRESULT hr = S_OK;
		IStream *stream = NULL;

		switch (pWiaTransferParams->lMessage)
		{
		case WIA_TRANSFER_MSG_STATUS:
			break;
		case WIA_TRANSFER_MSG_END_OF_STREAM:
			//...
			stream = m_streams.front();
			m_streams.pop();
			break;
		case WIA_TRANSFER_MSG_END_OF_TRANSFER:
			break;
		default:
			break;
		}

		if (!m_statusCallback(
			pWiaTransferParams->lMessage,
			pWiaTransferParams->lPercentComplete,
			pWiaTransferParams->ulTransferredBytes,
			pWiaTransferParams->hrErrorStatus,
			stream))
		{
			hr = S_FALSE;
		}

		if (stream)
		{
			stream->Release();
		}

		return hr;
	}

	HRESULT __stdcall GetNextStream(
		LONG    lFlags,
		BSTR    bstrItemName,
		BSTR    bstrFullItemName,
		IStream **ppDestination)
	{

		HRESULT hr = S_OK;

		//  Return a new stream for this item's data.
		//
		IStream *stream = SHCreateMemStream(nullptr, 0);
		*ppDestination = stream;
		m_streams.push(stream);
		stream->AddRef();
		return hr;
	}

public: // IUnknown
	//... // Etc.

	HRESULT CALLBACK QueryInterface(REFIID riid, void **ppvObject) override
	{
		// Validate arguments
		if (NULL == ppvObject)
		{
			return E_INVALIDARG;
		}

		// Return the appropropriate interface
		if (IsEqualIID(riid, IID_IUnknown))
		{
			*ppvObject = static_cast<IUnknown*>(this);
		}
		else if (IsEqualIID(riid, IID_IWiaTransferCallback))
		{
			*ppvObject = static_cast<IWiaTransferCallback*>(this);
		}
		else
		{
			*ppvObject = NULL;
			return (E_NOINTERFACE);
		}

		// Increment the reference count before we return the interface
		reinterpret_cast<IUnknown*>(*ppvObject)->AddRef();
		return S_OK;
	}

	ULONG CALLBACK AddRef() override
	{
		return InterlockedIncrement((long*)&m_cRef);
	}

	ULONG CALLBACK Release() override
	{
		LONG cRef = InterlockedDecrement((long*)&m_cRef);
		if (0 == cRef)
		{
			delete this;
		}
		return cRef;
	}

private:
	LONG m_cRef;
	std::queue<IStream*> m_streams;
	std::function<bool(LONG, LONG, ULONG64, HRESULT, IStream*)> m_statusCallback;
};

HRESULT DoEnum(IWiaItem *item, IEnumWiaItem **enumerator)
{
	return item->EnumChildItems(enumerator);
}

HRESULT DoEnum(IWiaItem2 *item, IEnumWiaItem2 **enumerator)
{
	return item->EnumChildItems(NULL, enumerator);
}

template<class TItem, class TEnum> HRESULT EnumerateItems(TItem *item, void __stdcall itemCallback(TItem*))
{
	LONG itemType = 0;
	HRESULT hr = item->GetItemType(&itemType);
	if (SUCCEEDED(hr))
	{
		if (itemType & WiaItemTypeFolder || itemType & WiaItemTypeHasAttachments)
		{
			TEnum *enumerator = NULL;
			hr = DoEnum(item, &enumerator);

			if (SUCCEEDED(hr))
			{
				while (S_OK == hr)
				{
					TItem *childItem = NULL;
					hr = enumerator->Next(1, &childItem, NULL);

					if (S_OK == hr)
					{
						itemCallback(childItem);
					}
				}
				if (hr == S_FALSE)
				{
					hr = S_OK;
				}
				enumerator->Release();
				enumerator = NULL;
			}
		}
	}
	return hr;
}

template<class TMgr> HRESULT __stdcall EnumerateDevices(TMgr *deviceManager, void __stdcall deviceCallback(IWiaPropertyStorage*))
{
	IEnumWIA_DEV_INFO *enumDevInfo;
	HRESULT hr = deviceManager->EnumDeviceInfo(WIA_DEVINFO_ENUM_LOCAL, &enumDevInfo);
	if (SUCCEEDED(hr))
	{
		while (hr == S_OK)
		{
			IWiaPropertyStorage *propStorage = NULL;
			hr = enumDevInfo->Next(1, &propStorage, NULL);
			if (hr == S_OK)
			{
				deviceCallback(propStorage);
			}
		}
		if (hr == S_FALSE)
		{
			hr = S_OK;
		}
	}
	return hr;
}

extern "C" {

	__declspec(dllexport) HRESULT __stdcall GetDeviceManager1(IWiaDevMgr **devMgr)
	{
		return CoCreateInstance(CLSID_WiaDevMgr, NULL, CLSCTX_LOCAL_SERVER, IID_IWiaDevMgr, (void**)devMgr);
	}

	__declspec(dllexport) HRESULT __stdcall GetDeviceManager2(IWiaDevMgr2 **devMgr)
	{
		return CoCreateInstance(CLSID_WiaDevMgr2, NULL, CLSCTX_LOCAL_SERVER, IID_IWiaDevMgr2, (void**)devMgr);
	}

	__declspec(dllexport) HRESULT __stdcall GetDevice1(IWiaDevMgr *devMgr, BSTR deviceId, IWiaItem **device)
	{
		return devMgr->CreateDevice(deviceId, device);
	}

	__declspec(dllexport) HRESULT __stdcall GetDevice2(IWiaDevMgr2 *devMgr, BSTR deviceId, IWiaItem2 **device)
	{
		return devMgr->CreateDevice(0, deviceId, device);
	}

	__declspec(dllexport) HRESULT __stdcall SetPropertyInt(IWiaPropertyStorage *propStorage, int propId, int value)
	{
		PROPSPEC PropSpec[1] = { 0 };
		PROPVARIANT PropVariant[1] = { 0 };
		PropSpec[0].ulKind = PRSPEC_PROPID;
		PropSpec[0].propid = propId;
		PropVariant[0].vt = VT_I4;
		PropVariant[0].lVal = value;

		return propStorage->WriteMultiple(1, PropSpec, PropVariant, WIA_IPA_FIRST);
	}

	__declspec(dllexport) HRESULT __stdcall GetPropertyBstr(IWiaPropertyStorage *propStorage, int propId, BSTR *value)
	{
		PROPSPEC PropSpec[1] = { 0 };
		PROPVARIANT PropVariant[1] = { 0 };
		PropSpec[0].ulKind = PRSPEC_PROPID;
		PropSpec[0].propid = propId;

		HRESULT hr = propStorage->ReadMultiple(1, PropSpec, PropVariant);
		*value = PropVariant[0].bstrVal;
		return hr;
	}

	__declspec(dllexport) HRESULT __stdcall GetPropertyInt(IWiaPropertyStorage *propStorage, int propId, int *value)
	{
		PROPSPEC PropSpec[1] = { 0 };
		PROPVARIANT PropVariant[1] = { 0 };
		PropSpec[0].ulKind = PRSPEC_PROPID;
		PropSpec[0].propid = propId;

		HRESULT hr = propStorage->ReadMultiple(1, PropSpec, PropVariant);
		*value = PropVariant[0].lVal;
		return hr;
	}

	__declspec(dllexport) HRESULT __stdcall GetPropertyAttributes(IWiaPropertyStorage *propStorage, int propId, int *flags, int *min, int *nom, int *max, int *step, int *numElems, int **elems)
	{
		PROPSPEC PropSpec[1] = { 0 };
		ULONG PropFlags[1] = { 0 };
		PROPVARIANT PropVariant[1] = { 0 };
		PropSpec[0].ulKind = PRSPEC_PROPID;
		PropSpec[0].propid = propId;

		HRESULT hr = propStorage->GetPropertyAttributes(1, PropSpec, PropFlags, PropVariant);
		
		*flags = PropFlags[0];
		if ((*flags & WIA_PROP_RANGE) == WIA_PROP_RANGE)
		{
			*min = PropVariant[0].caul.pElems[WIA_RANGE_MIN];
			*nom = PropVariant[0].caul.pElems[WIA_RANGE_NOM];
			*max = PropVariant[0].caul.pElems[WIA_RANGE_MAX];
			*step = PropVariant[0].caul.pElems[WIA_RANGE_STEP];
		}
		if ((*flags & WIA_PROP_LIST) == WIA_PROP_LIST)
		{
			*numElems = PropVariant[0].caul.cElems;
			*nom = PropVariant[0].caul.pElems[WIA_LIST_NOM];
			*elems = (int*)PropVariant[0].caul.pElems;
		}
		
		return hr;
	}

	__declspec(dllexport) HRESULT __stdcall EnumerateItems1(IWiaItem *item, void __stdcall itemCallback(IWiaItem*))
	{
		return EnumerateItems<IWiaItem, IEnumWiaItem>(item, itemCallback);
	}

	__declspec(dllexport) HRESULT __stdcall EnumerateItems2(IWiaItem2 *item, void __stdcall itemCallback(IWiaItem2*))
	{
		return EnumerateItems<IWiaItem2, IEnumWiaItem2>(item, itemCallback);
	}

	__declspec(dllexport) HRESULT __stdcall StartTransfer1(IWiaItem *item, IWiaDataTransfer **transfer)
	{
		PROPSPEC PropSpec[2] = { 0 };
		PROPVARIANT PropVariant[2] = { 0 };
		GUID guidOutputFormat = WiaImgFmt_BMP;
		PropSpec[0].ulKind = PRSPEC_PROPID;
		PropSpec[0].propid = WIA_IPA_FORMAT;
		PropSpec[1].ulKind = PRSPEC_PROPID;
		PropSpec[1].propid = WIA_IPA_TYMED;
		PropVariant[0].vt = VT_CLSID;
		PropVariant[0].puuid = &guidOutputFormat;
		PropVariant[1].vt = VT_I4;
		PropVariant[1].lVal = TYMED_CALLBACK;
		IWiaPropertyStorage* propStorage;
		HRESULT hr = item->QueryInterface(IID_IWiaPropertyStorage, (void**)&propStorage);
		hr = propStorage->WriteMultiple(2, PropSpec, PropVariant, WIA_IPA_FIRST);

		return item->QueryInterface(IID_IWiaDataTransfer, (void**)transfer);
	}

	__declspec(dllexport) HRESULT __stdcall StartTransfer2(IWiaItem2 *item, IWiaTransfer **transfer)
	{
		return item->QueryInterface(IID_IWiaTransfer, (void**)transfer);
	}

	__declspec(dllexport) HRESULT __stdcall Download1(IWiaDataTransfer *transfer, bool __stdcall statusCallback(LONG, LONG, ULONG64, HRESULT, IStream*))
	{
		CWiaTransferCallback1 *callbackClass = new CWiaTransferCallback1(statusCallback);
		if (callbackClass)
		{
			/*STGMEDIUM StgMedium = { 0 };
			StgMedium.tymed = TYMED_ISTREAM;
			StgMedium.pstm*/
			WIA_DATA_TRANSFER_INFO WiaDataTransferInfo = { 0 };
			WiaDataTransferInfo.ulSize = sizeof(WIA_DATA_TRANSFER_INFO);
			WiaDataTransferInfo.ulBufferSize = 131072 * 2;
			WiaDataTransferInfo.bDoubleBuffer = TRUE;
			return transfer->idtGetBandedData(&WiaDataTransferInfo, callbackClass);
		}
		return S_FALSE;
	}

	__declspec(dllexport) HRESULT __stdcall Download2(IWiaTransfer *transfer, bool __stdcall statusCallback(LONG, LONG, ULONG64, HRESULT, IStream*))
	{
		CWiaTransferCallback2 *callbackClass = new CWiaTransferCallback2(statusCallback);
		if (callbackClass)
		{
			return transfer->Download(0, callbackClass);
		}
		return S_FALSE;
	}

	__declspec(dllexport) HRESULT __stdcall EnumerateDevices1(IWiaDevMgr *deviceManager, void __stdcall deviceCallback(IWiaPropertyStorage*))
	{
		return EnumerateDevices(deviceManager, deviceCallback);
	}

	__declspec(dllexport) HRESULT __stdcall EnumerateDevices2(IWiaDevMgr2 *deviceManager, void __stdcall deviceCallback(IWiaPropertyStorage*))
	{
		return EnumerateDevices(deviceManager, deviceCallback);
	}

	__declspec(dllexport) HRESULT __stdcall GetItemPropertyStorage(IUnknown *item, IWiaPropertyStorage** propStorage)
	{
		return item->QueryInterface(IID_IWiaPropertyStorage, (void**)propStorage);
	}

	__declspec(dllexport) HRESULT __stdcall EnumerateProperties(IWiaPropertyStorage *propStorage, void __stdcall propCallback(int, LPOLESTR, VARTYPE))
	{
		IEnumSTATPROPSTG *enumProps;
		HRESULT hr = propStorage->Enum(&enumProps);
		if (SUCCEEDED(hr))
		{
			while (hr == S_OK)
			{
				STATPROPSTG prop;
				hr = enumProps->Next(1, &prop, NULL);
				if (hr == S_OK)
				{
					propCallback(prop.propid, prop.lpwstrName, prop.vt);
				}
			}
			if (hr == S_FALSE)
			{
				hr = S_OK;
			}
		}
		return hr;
	}

	__declspec(dllexport) HRESULT __stdcall SelectDevice1(IWiaDevMgr *deviceManager, HWND hwnd, LONG deviceType, LONG flags, BSTR *deviceId, IWiaItem **device)
	{
		return deviceManager->SelectDeviceDlg(hwnd, deviceType, flags, deviceId, device);
	}

	__declspec(dllexport) HRESULT __stdcall SelectDevice2(IWiaDevMgr2 *deviceManager, HWND hwnd, LONG deviceType, LONG flags, BSTR *deviceId, IWiaItem2 **device)
	{
		return deviceManager->SelectDeviceDlg(hwnd, deviceType, flags, deviceId, device);
	}

	__declspec(dllexport) HRESULT __stdcall GetImage1(IWiaDevMgr *deviceManager, HWND hwnd, LONG deviceType, LONG flags, LONG intent, BSTR filepath, IWiaItem *item)
	{
		GUID format = WiaImgFmt_JPEG;
		return deviceManager->GetImageDlg(hwnd, deviceType, flags, intent, item, filepath, &format);
	}

	__declspec(dllexport) HRESULT __stdcall GetImage2(IWiaDevMgr2 *deviceManager, LONG flags, BSTR deviceId, HWND hwnd, BSTR folder, BSTR filename, LONG *numFiles, BSTR **filePaths, IWiaItem2 **item)
	{
		return deviceManager->GetImageDlg(flags, deviceId, hwnd, folder, filename, numFiles, filePaths, item);
	}

	__declspec(dllexport) HRESULT __stdcall ConfigureDevice1(IWiaItem *device, HWND hwnd, LONG flags, LONG intent, LONG *itemCount, IWiaItem ***items)
	{
		return device->DeviceDlg(hwnd, flags, intent, itemCount, items);
	}

	__declspec(dllexport) HRESULT __stdcall ConfigureDevice2(IWiaItem2 *device, LONG flags, HWND hwnd, BSTR folder, BSTR filename, LONG *numFiles, BSTR **filePaths, IWiaItem2 **items)
	{
		return device->DeviceDlg(flags, hwnd, folder, filename, numFiles, filePaths, items);
	}
}
// Ds7.cpp
//
// Generated by DriverWizard version DriverStudio 2.7.0 (Build 562)
// Requires Compuware's DriverWorks classes
//

#define VDW_MAIN
#include <vdw.h>
#include <kusb.h>
#include "Ds7.h"
#include "Ds7Device.h"

#pragma hdrstop("Ds7.pch")

// Generated by DriverWizard version DriverStudio 2.7.0 (Build 562)

// Set a default 32-bit tag value to be stored with each heap block
// allocated by operator new. Use BoundsChecker to view the memory pool.
// This value can be overridden using the global function SetPoolTag().
POOLTAG DefaultPoolTag(' 7sD');

// Create the global driver trace object
// TODO:	Use KDebugOnlyTrace if you want trace messages
//			to appear only in debug builds.  Use KTrace if
//			you want trace messages to always appear.	
KTrace t("Ds7");

/////////////////////////////////////////////////////////////////////
// Begin INIT section
#pragma code_seg("INIT")

DECLARE_DRIVER_CLASS(Ds7, NULL)

/////////////////////////////////////////////////////////////////////
//  Ds7::DriverEntry
//
//	Routine Description:
//		This is the first entry point called by the system when the
//		driver is loaded.
// 
//	Parameters:
//		RegistryPath - String used to find driver parameters in the
//			registry.  To locate Ds7 look for:
//			HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\Ds7
//
//	Return Value:
//		NTSTATUS - Return STATUS_SUCCESS if no errors are encountered.
//			Any other indicates to the system that an error has occured.
//
//	Comments:
//

NTSTATUS Ds7::DriverEntry(PUNICODE_STRING RegistryPath)
{
	t << "In DriverEntry Compiled at " __TIME__ " on " __DATE__ "\n";


	// Open the "Parameters" key under the driver
	KRegistryKey Params(RegistryPath, L"Parameters");
	if ( NT_SUCCESS(Params.LastError()) )
	{
#if DBG
		ULONG bBreakOnEntry = FALSE;
		// Read "BreakOnEntry" value from registry
		Params.QueryValue(L"BreakOnEntry", &bBreakOnEntry);
		// If requested, break into debugger
		if (bBreakOnEntry) DbgBreakPoint();
#endif
		// Load driver data members from the registry
		LoadRegistryParameters(Params);
	}
	m_Unit = 0;

	return STATUS_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//  Ds7::LoadRegistryParameters
//
//	Routine Description:
//		Load driver data members from the registry.
// 
//	Parameters:
//		Params - Open registry key pointing to "Parameters"
//
//	Return Value:
//		None
//			
//	Comments:
//		Member variables are updated with values read from registry.
//
//		The parameters are found as values under the "Parameters" key,	
//		HKLM\SYSTEM\CurrentControlSet\Services\Ds7\Parameters\...
//

void Ds7::LoadRegistryParameters(KRegistryKey &Params)
{

	m_bBreakOnEntry = FALSE;
	Params.QueryValue(L"BreakOnEntry", &m_bBreakOnEntry);
	t << "m_bBreakOnEntry loaded from registry, resulting value: [" << m_bBreakOnEntry << "]\n";

}
// End INIT section
/////////////////////////////////////////////////////////////////////
#pragma code_seg()

/////////////////////////////////////////////////////////////////////
//  Ds7::AddDevice
//
//	Routine Description:
//		Called when the system detects a device for which this
//		driver is responsible.
//
//	Parameters:
//		Pdo - Physical Device Object. This is a pointer to a system device
//			object that represents the physical device.
//
//	Return Value:
//		NTSTATUS - Success or failure code.
//
//	Comments:
//		This function creates the Functional Device Object, or FDO. The FDO
//		enables this driver to handle requests for the physical device. 
//

NTSTATUS Ds7::AddDevice(PDEVICE_OBJECT Pdo)
{
	t << "AddDevice called\n";

    // Create the device object. Note that we used a form of "placement" new,
	// that is a member operator of KDevice.  This form will use storage
	// allocated by the system in the device object's device to store our
	// class instance.
	Ds7Device * pDevice = new (
			static_cast<PCWSTR>(KUnitizedName(L"Ds7Device", m_Unit)),
			FILE_DEVICE_UNKNOWN,
			NULL,
			0,
			DO_DIRECT_IO
				| DO_POWER_PAGABLE
			)
		Ds7Device(Pdo, m_Unit);

	if (pDevice == NULL)
	{
		t << "Error creating device Ds7Device"
			   << (ULONG) m_Unit << EOL;
	    return STATUS_INSUFFICIENT_RESOURCES;
	}

	NTSTATUS status = pDevice->ConstructorStatus();

	if ( !NT_SUCCESS(status) )
	{
		t << "Error constructing device Ds7Device"
		  << (ULONG) m_Unit << " status " << (ULONG) status << EOL;
		delete pDevice;
	}
	else
	{
		m_Unit++;

		pDevice->ReportNewDevicePowerState(PowerDeviceD0);
	}

	return status;
}

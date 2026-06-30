/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    usbd.h

Abstract:

    Private header for USBD internal modules.

    The shared USB/URB/HCD definitions (URB structures, URB_FUNCTION_* codes,
    USBD status codes, the HCD<->USBD interface) live in hcdi.h and were
    formerly duplicated here verbatim; this header now includes hcdi.h and
    adds only the USBD-internal C++ surface.

Environment:

    Xbox
    C++ Only

--*/

#pragma once

#ifndef __cplusplus
#error "usb.h may only be included by C++ modules"
#endif

#define EXTERNUSB extern "C"

//
// Shared USB/URB/HCD definitions. hcdi.h self-guards its C linkage, so it can
// be included directly here (C++) and by the OHCD (C) translation units.
//
#include "hcdi.h"

//------------------------------------------------------------------------------------
//  Forward declaration of classes
//------------------------------------------------------------------------------------
class IUsbDevice;
class CDeviceTree;

//---------------------------------------------------------------------------------------------------------------
// USBD_HOST_CONTROLLER - Context of USBD by host controller.  Instead of a DEVICE_OBJECT
//---------------------------------------------------------------------------------------------------------------
#pragma warning(disable : 4200) //Turn off zero length array warning
typedef struct _USBD_HOST_CONTROLLER
{
    ULONG           ControllerNumber;   //Number of this controller.
    IUsbDevice      *RootHub;           //Root Hub
    ULONG           AddressList[4];     //128 bit bitfield of used USB addresses.
    ULONG           HcdExtension[0];    //The Host controllers extension starts here.
} USBD_HOST_CONTROLLER, *PUSBD_HOST_CONTROLLER;
#pragma warning(default : 4200) //Turn zero length array warning back on

//
//  Stuff related to the Host Controller Driver, its endpoint size and its extension
//
extern ULONG GLOBAL_HostControllerExtensionSize;
#define USBD_GetHCDExtension(_UsbdHostController_) ((PVOID)(_UsbdHostController_->HcdExtension))
#define USBD_HostControllerFromHCDExtension(_HCDExtension_)  CONTAINING_RECORD(_HCDExtension_, USBD_HOST_CONTROLLER, HcdExtension)

//---------------------------------------------------------------------------------------------------------------
//  Types of USB Device Nodes.
//      UDN_TYPE_HUB                - represents a hub device.
//      UDN_TYPE_FUNCTION           - a USB function for which bDeviceClass != 0.
//      UDN_TYPE_INTERFACE_FUNCTION - a USB function where bDeviceClass is 0, but there is only one interface.
//      UDN_TYPE_COMPOSITE_FUNCTION - a USB function where bDeviceClass is 0, and there are multiple interfaces.
//                                    a COMPOSITE_FUNCTION always has one INTERFACE child be interface.   
//      UDN_TYPE_INTERFACE          - represents an INTERFACE on a COMPOSITE_FUNCTION.
//===============================================================================================================
//  Special types not signifying a device
//      UDN_TYPE_UNUSED             - Not currently representing a device.  Node should be on the free list.
//      UDN_TYPE_PENDING_ENUM       - Device has been detected, but is in the list of nodes awaiting enumeration.
//      UDN_TYPE_ENUMERATING        - Device is currently being enumerated, but has not reached a stage of enumeration
//                                    yet where the node type is known.
//---------------------------------------------------------------------------------------------------------------
#define UDN_TYPE_ROOT_HUB           0
#define UDN_TYPE_HUB                1
#define UDN_TYPE_FUNCTION           2
#define UDN_TYPE_INTERFACE_FUNCTION 3
#define UDN_TYPE_COMPOSITE_FUNCTION 4
#define UDN_TYPE_INTERFACE          5
#define UDN_TYPE_UNUSED             0xFF    
#define UDN_TYPE_PENDING_ENUM       0xFE
#define UDN_TYPE_ENUMERATING        0xFD

//---------------------------------------------------------------------------------------------------------------
//  Device node constants
//---------------------------------------------------------------------------------------------------------------
#define UDN_INVALID_NODE_INDEX      128   // Indicates that index does not point to a valid node.

#define UDN_LOWSPEED_PORT           0x80
#define UDN_NO_CLASS_DRIVER_SUPPORT 0xFF
#define UDN_HUB_TYPE_EXTERNAL       0x01

//--------------------------------------------------------------------------------------------------------------
// Resource Requirements Structure
//--------------------------------------------------------------------------------------------------------------
typedef struct _USB_RESOURCE_REQUIREMENTS
{
    UCHAR ConnectorType;
    UCHAR MaxDevices;
    UCHAR MaxCompositeInterfaces;
    UCHAR MaxControlEndpoints;
    UCHAR MaxBulkEndpoints;
    UCHAR MaxInterruptEndpoints;
    UCHAR MaxControlTDperTransfer;
    UCHAR MaxBulkTDperTransfer;
    //Isoch
    UCHAR MaxIsochEndpoints;
    UCHAR MaxIsochMaxBuffers;
} USB_RESOURCE_REQUIREMENTS, *PUSB_RESOURCE_REQUIREMENTS;
//-------------------------------------------------------------------------------------------------
// This class is used to assist initialization.
//-------------------------------------------------------------------------------------------------
class IUsbInit
{
  public:
    ULONG GetMaxDeviceTypeCount(PXPP_DEVICE_TYPE XppDeviceType);
    VOID  RegisterResources(PUSB_RESOURCE_REQUIREMENTS pResourceRequirements);
    BOOL  UseDefaultCount();
    //c'tor
    IUsbInit(ULONG NumDeviceTypes, PXDEVICE_PREALLOC_TYPE DeviceTypes) :
      m_NumDeviceTypes(NumDeviceTypes), m_pDeviceTypes(DeviceTypes),
      m_NodeCount(0), m_MaxCompositeInterfaces(0)
      {
        RtlZeroMemory(m_Direct, sizeof(m_Direct));
        RtlZeroMemory(m_TopSlots, sizeof(m_TopSlots));
        RtlZeroMemory(m_BottomSlots, sizeof(m_BottomSlots));
        RtlZeroMemory(&m_HcdResources, sizeof(m_HcdResources));
      }

    //services for usbd to retrieve information
    void Process();
    inline UCHAR GetNodeCount() {return m_NodeCount;}
    inline UCHAR GetMaxCompositeInterfaces() {return m_MaxCompositeInterfaces;}
    inline PHCD_RESOURCE_REQUIREMENTS GetHcdResourcePtr() {return &m_HcdResources;}
  private:
    USB_RESOURCE_REQUIREMENTS m_Direct[5];
    USB_RESOURCE_REQUIREMENTS m_TopSlots[5];
    USB_RESOURCE_REQUIREMENTS m_BottomSlots[5];
    ULONG                     m_NumDeviceTypes;
    PXDEVICE_PREALLOC_TYPE    m_pDeviceTypes;
    // Fields valid after process
    UCHAR                     m_NodeCount;
    UCHAR                     m_MaxCompositeInterfaces;
    HCD_RESOURCE_REQUIREMENTS m_HcdResources;
};
//--------------------------------------------------------------------------------------------------------------
// Class Driver Static Registration 
//--------------------------------------------------------------------------------------------------------------
typedef union _PNP_CLASS_ID
{
    LONG AsLong;
    struct
    {
        UCHAR bClassSpecificType;
        UCHAR bClass;
        UCHAR bSubClass;
        UCHAR bProtocol;
    } USB;
} PNP_CLASS_ID, *PPNP_CLASS_ID;

// The high-bit of bClassSpecificType is set
// if the class driver has not indicated a class
// specific type.  Prior to calling AddDevice
// this is used to track whether this is a 
// device level or interface level class.
// 
// USB_CLASS_DRIVER_DECLARATION assumes PNP_INTERFACE_LEVEL_CLASS
// 
#define PNP_CLASS_SPECIFIC_TYPE_NOT_SET(bClassSpecificType) (bClassSpecificType&0x80)
#define PNP_DEVICE_LEVEL_CLASS    0x81
#define PNP_INTERFACE_LEVEL_CLASS 0x82

typedef
VOID
 (*PFNINIT_USB_DRIVER)(IUsbInit *UsbInit);

typedef
VOID
 (*PFNADD_USB_DEVICE)(
    IN IUsbDevice *Device
    );

typedef
VOID
 (*PFNREMOVE_USB_DEVICE)(
    IN IUsbDevice *Device
    );


typedef struct _USB_DEVICE_TYPE_DESCRIPTION
{
    PXPP_DEVICE_TYPE XppDeviceType;
} USB_DEVICE_TYPE_DESCRIPTION, *PUSB_DEVICE_TYPE_DESCRIPTION;

#define USB_CONNECTOR_TYPE_DIRECT     0  //Plugs directly into the front of xbox (or a hub port)
#define USB_CONNECTOR_TYPE_HIGH_POWER 1  //Plugs into a high power slot in gamepad
#define USB_CONNECTOR_TYPE_LOW_POWER  2  //Plugs into a high power or low power slot in gamepad

typedef struct _USB_CLASS_DRIVER_DESCRIPTION
{
    PNP_CLASS_ID                 ClassId;
    PFNINIT_USB_DRIVER           Init;
    PFNADD_USB_DEVICE            AddDevice;
    PFNREMOVE_USB_DEVICE         RemoveDevice;
    ULONG                        DeviceTypeCount;
    PXPP_DEVICE_TYPE             *DeviceTypes;
} USB_CLASS_DRIVER_DESCRIPTION, *PUSB_CLASS_DRIVER_DESCRIPTION;

#define DECLARE_XPP_TYPE(XppTypeName)\
EXTERNUSB XPP_DEVICE_TYPE XppTypeName##_TABLE = {0,0,0};

#define USB_DEVICE_TYPE_TABLE_BEGIN(ClassName)\
EXTERNUSB PXPP_DEVICE_TYPE ClassName##Types[]={

#define USB_DEVICE_TYPE_TABLE_ENTRY(XppDeviceType)\
                 (XppDeviceType)

#define USB_DEVICE_TYPE_TABLE_END() };

#define USB_CLASS_DRIVER_DECLARATION(ClassName, bClass, bSubClass, bProtocol)\
               EXTERNUSB VOID ClassName##Init(IUsbInit *UsbInit);\
               EXTERNUSB VOID ClassName##AddDevice(IUsbDevice *Device);\
               EXTERNUSB VOID ClassName##RemoveDevice(IUsbDevice *Device);\
               EXTERNUSB USB_CLASS_DRIVER_DESCRIPTION ClassName##Description = {\
                    PNP_INTERFACE_LEVEL_CLASS + ((bClass << 8) + (bSubClass << 16) + (bProtocol << 24)),\
                    ClassName##Init,\
                    ClassName##AddDevice,\
                    ClassName##RemoveDevice,\
                    sizeof(ClassName##Types)/sizeof(USB_DEVICE_TYPE_DESCRIPTION),\
                    ClassName##Types\
                   };

#define USB_CLASS_DRIVER_DECLARATION_DUPLICATE(ClassName, DuplicateNumber, bClass, bSubClass, bProtocol)\
               EXTERNUSB USB_CLASS_DRIVER_DESCRIPTION ClassName##DuplicateNumber##Description = {\
                    PNP_INTERFACE_LEVEL_CLASS + ((bClass << 8) + (bSubClass << 16) + (bProtocol << 24)),\
                    ClassName##Init,\
                    ClassName##AddDevice,\
                    ClassName##RemoveDevice,\
                    sizeof(ClassName##Types)/sizeof(USB_DEVICE_TYPE_DESCRIPTION),\
                    ClassName##Types\
                   };

#define USB_CLASS_DRIVER_DECLARATION_DEVICE_LEVEL(ClassName, bClass, bSubClass, bProtocol)\
               EXTERNUSB VOID ClassName##Init(IUsbInit *UsbInit);\
               EXTERNUSB VOID ClassName##AddDevice(IUsbDevice *Device);\
               EXTERNUSB VOID ClassName##RemoveDevice(IUsbDevice *Device);\
               EXTERNUSB USB_CLASS_DRIVER_DESCRIPTION ClassName##Description = {\
                    PNP_DEVICE_LEVEL_CLASS + ((bClass << 8) + (bSubClass << 16) + (bProtocol << 24)),\
                    ClassName##Init,\
                    ClassName##AddDevice,\
                    ClassName##RemoveDevice,\
                    sizeof(ClassName##Types)/sizeof(USB_DEVICE_TYPE_DESCRIPTION),\
                    ClassName##Types\
                   };

#define USB_CLASS_DECLARATION_POINTER(ClassName)\
                EXTERNUSB USB_CLASS_DRIVER_DESCRIPTION *ClassName##DescriptionPointer=&ClassName##Description;
#define USB_CLASS_DECLARATION_POINTER_DUPLICATE(ClassName, DuplicateNumber)\
                EXTERNUSB USB_CLASS_DRIVER_DESCRIPTION *ClassName##DuplicateNumber##DescriptionPointer=\
                &ClassName##DuplicateNumber##Description;

#define REFERENCE_CLASS(ClassName)\
    EXTERNUSB USB_CLASS_DRIVER_DESCRIPTION ClassName##Description;\
    static USB_CLASS_DRIVER_DESCRIPTION *classMU = &(ClassName##Description);

//---------------------------------------------------------------------------------------------------------------
//  IUsbDevice is the main interface to the core driver
//---------------------------------------------------------------------------------------------------------------
class IUsbDevice
{
    public:
    /*  IUsbDevice methods calleable at any time by class driver*/
    USBD_STATUS SubmitRequest(PURB Urb);
    USBD_STATUS CancelRequest(PURB Urb);
    BOOLEAN IsHardwareConnected() const;
    PVOID GetExtension() const;
    PVOID SetExtension(PVOID Extension);
    UCHAR GetInterfaceNumber() const;
	void SetClassSpecificType(UCHAR ClassSpecificType);
    ULONG GetPort() const;

    /*  IUsbDevice methods related to device enumeration*/
    void AddComplete(USBD_STATUS UsbdStatus);
    void RemoveComplete();
    void DeviceNotResponding();

    /*  IUsbDevice methods calleable only at enum time*/
    const USB_DEVICE_DESCRIPTOR8 *GetDeviceDescriptor() const;
    const USB_CONFIGURATION_DESCRIPTOR *GetConfigurationDescriptor() const;
    const USB_INTERFACE_DESCRIPTOR *GetInterfaceDescriptor() const;
    const USB_ENDPOINT_DESCRIPTOR *GetEndpointDescriptor(UCHAR EndpointType, BOOLEAN Direction, UCHAR Index) const;
    
	/*  IUsbDevice methods used only by hubs */
    void DeviceConnected(UCHAR PortNumber, UCHAR RetryCount);
    void DeviceDisconnected(UCHAR PortNumber);
    void ResetComplete(USBD_STATUS UsbdStatus, PVOID Context);
    void DisableComplete(USBD_STATUS UsbdStatus, PVOID Context);

    /* static IUsbDevice methods */
    static ULONG Win32FromUsbdStatus(USBD_STATUS UsbdStatus);
	static NTSTATUS NtStatusFromUsbdStatus(USBD_STATUS UsbdStatus);
    /*  c'tor */
    IUsbDevice() :
        m_Type(UDN_TYPE_UNUSED), m_NextFree(UDN_INVALID_NODE_INDEX),
        m_FirstChild(UDN_INVALID_NODE_INDEX), m_Sibling(UDN_INVALID_NODE_INDEX){}

    /* IUsbDevice short and sweet methods used in USBD, these are inline, but defined below due to declaration order */
    UCHAR			GetIndex() const;
    BOOLEAN			GetLowSpeed() const;
    UCHAR			GetHubPort() const;
    IUsbDevice		*GetParent() const;
    IUsbDevice		*GetFirstChild() const;
    IUsbDevice		*GetSibling() const;
    IUsbDevice		*FindChild(UCHAR PortNumber) const;
    void			InsertChild(IUsbDevice *child);
    BOOLEAN			RemoveChild(IUsbDevice *child);
    BOOLEAN			IsEnumTime() const;
    PNP_CLASS_ID    GetClassId() const;
    /* public helper function*/
    void            SetExternalPort();
	#ifndef SILVER
	void            SetExternalPortWithHub(IUsbDevice **pParentArray, UINT DeviceIndex);
	void            SetExternalPortWithoutHub(IUsbDevice **pParentArray, UINT DeviceIndex);
	#endif

    //
    //  These are not declared private as most of the code is already coded in C
    //  and manipulates this class directly.
    //
    UCHAR   m_Type;
    union
    {
        UCHAR   m_Parent;
        UCHAR   m_NextFree;
    };
    union
    {
        UCHAR   m_FirstChild;
        UCHAR   m_bInterfaceNumber;
    };
    UCHAR           m_Sibling;

    UCHAR           m_PortNumber;
    union
    {
        UCHAR   m_Address;        
        UCHAR   m_RetryCount;
    };
    UCHAR           m_MaxPacket0;
    UCHAR           m_ClassSpecificType;

    PVOID           m_DefaultEndpoint;
    PUSBD_HOST_CONTROLLER m_HostController;

    union
    {
        PUSB_CLASS_DRIVER_DESCRIPTION   m_ClassDriver;
        IUsbDevice                      *m_NextPending;
    };
    LONG            m_ExternalPort;
    union
    {
        LARGE_INTEGER  m_EarliestEnumTime;
        struct
        {
            ULONG           m_DataToggleBits;
            PVOID			m_ClassDriverExtension;
        };
    };
    
    /* IUsbDevice helpers, these are only called internally so can be private */
    private:
    USBD_STATUS OpenDefaultEndpoint(PURB Urb);
    USBD_STATUS CloseDefaultEndpoint(PURB Urb);
};
//--------------------------------------------------------------------------------
//  Hub driver must implement this method
//--------------------------------------------------------------------------------
extern VOID USBHUB_DisableResetPort(
	                    IN IUsbDevice *HubDevice,
	                    IN UCHAR PortNumber,
	                    IN PVOID Context,
                        IN BOOLEAN Disable
	                    );
//--------------------------------------------------------------------------------
//  Enum Stages, mostly for debugging really
//--------------------------------------------------------------------------------
#define USBD_ENUM_DEVICE_CONNECTED 0x80
#define USBD_ENUM_STAGE_0 0
#define USBD_ENUM_STAGE_PRE1 0x81
#define USBD_ENUM_STAGE_1 1
#define USBD_ENUM_STAGE_2 2
#define USBD_ENUM_STAGE_3 3
#define USBD_ENUM_STAGE_PRE4 0x84
#define USBD_ENUM_STAGE_4 4
#define USBD_ENUM_STAGE_5 5
#define USBD_ENUM_STAGE_6 6
#define USBD_ENUM_STAGE_ADD_COMPLETE 7
#define USBD_ENUM_STAGE_ABORT1 8
#define USBD_ENUM_STAGE_DISABLE_COMPLETE 9
#define USBD_ENUM_STAGE_ABORT2 10

//--------------------------------------------------------------------------------
//  CDeviceTree keeps track of all the devices
//--------------------------------------------------------------------------------
#define USBD_MAX_CONFIG_DESC_SIZE     80
#define USBD_DEFAULT_MAXPACKET0       8
#define USBD_CONTROL_TD_QUOTA         (USBD_MAX_CONFIG_DESC_SIZE/USBD_DEFAULT_MAXPACKET0) + 3 //the three is for SETUP, SETUP's data and STATUS
#define USBD_BASE_NODES_PER_PORT      4

class CDeviceTree
{
    public:

    //
    //  There is no defined c'tor, because there is no way
    //  to make sure that it gets called.
    //

    void Init(ULONG NodeCount, ULONG MaxCompositeInterfaces);
    IUsbDevice *AllocDevice() { 
            UCHAR nodeIndex = m_FirstFree;
            ASSERT(UDN_INVALID_NODE_INDEX != nodeIndex);
            m_FirstFree = m_Devices[nodeIndex].m_NextFree;
            m_Devices[nodeIndex].m_Parent = UDN_INVALID_NODE_INDEX;
            m_Devices[nodeIndex].m_FirstChild = UDN_INVALID_NODE_INDEX;
            m_Devices[nodeIndex].m_Sibling = UDN_INVALID_NODE_INDEX;
            m_Devices[nodeIndex].m_ClassDriverExtension = NULL;
            m_Devices[nodeIndex].m_ClassSpecificType = 0xFF;
            return m_Devices + nodeIndex;
            }
    VOID FreeDevice(IUsbDevice *usbDevice) { 
            ASSERT(usbDevice >= m_Devices);
            UCHAR nodeIndex = usbDevice - m_Devices;
            ASSERT(UDN_INVALID_NODE_INDEX > nodeIndex);
            m_Devices[nodeIndex].m_Type = UDN_TYPE_UNUSED;
            m_Devices[nodeIndex].m_NextFree = m_FirstFree;
            m_FirstFree = nodeIndex;
            }
    
    BOOLEAN                     m_InProgress;
    BOOLEAN                     m_DeviceRemoved;
    BOOLEAN                     m_RetryCount;
    UCHAR                       m_EnumStage;
    URB                         m_EnumUrb;
    KDPC                        m_EnumDpc;
    KTIMER                      m_EnumTimer;
    UCHAR                       m_TimerReason;
    UCHAR                       m_FirstFree;
    UCHAR                       m_NodeCount;
    UCHAR                       m_MaxCompositeInterfaces;
    IUsbDevice                  *m_FirstPendingEnum;
    IUsbDevice                  *m_CurrentEnum;
    USB_DEVICE_DESCRIPTOR8      m_DeviceDescriptor;
    UCHAR                       m_ConfigurationDescriptorBuffer[USBD_MAX_CONFIG_DESC_SIZE];
    PUSB_INTERFACE_DESCRIPTOR   m_InterfaceDescriptor;
    IUsbDevice                  *m_Devices;
};
extern CDeviceTree g_DeviceTree;
#define USBD_TIMER_REASON_STAGE0            0
#define USBD_TIMER_REASON_WATCHDOG          1
#define USBD_TIMER_REASON_CONTINUE_STAGE1   2
#define USBD_TIMER_REASON_CONTINUE_STAGE4   3
/**************************************************
***  Implementation of IUsbDevice inline functions
*********/
inline UCHAR
IUsbDevice::GetIndex() const
/*++
    Gets the index of this device in the global static tree.
--*/
{return (UCHAR)(this - g_DeviceTree.m_Devices);}

inline BOOLEAN
IUsbDevice::GetLowSpeed() const
/*++
    Returns true if the device is lowspeed.
    THIS WILL BE REMOVED WHEN SUPPORT FOR LOWSPEED IS DROPPED.
--*/
{return m_PortNumber & UDN_LOWSPEED_PORT ? TRUE : FALSE;}

inline UCHAR
IUsbDevice::GetHubPort() const
/*++
    Get the portnumber regardless of lowspeed or not.
--*/
{return m_PortNumber & ~UDN_LOWSPEED_PORT;}

inline IUsbDevice *
IUsbDevice::GetParent() const
/*++
    Returns a pointer to the parent.  NULL if there is no parent.
--*/
{return (UDN_INVALID_NODE_INDEX != m_Parent) ? (g_DeviceTree.m_Devices + m_Parent) : NULL;}

inline IUsbDevice *
IUsbDevice::GetFirstChild() const
/*++
    Returns a pointer to the first child.  NULL if there are no children.
--*/
{return (UDN_INVALID_NODE_INDEX != m_FirstChild) ? (g_DeviceTree.m_Devices + m_FirstChild) : NULL;}

inline IUsbDevice *
IUsbDevice::GetSibling() const
/*++
    Returns a pointer to the next sibling.  NULL if this is the last sibling.
--*/
{return (UDN_INVALID_NODE_INDEX != m_Sibling) ? (g_DeviceTree.m_Devices + m_Sibling) : NULL;}
   
//------------------------------------------------------------------------
//  Methods used across modules
//------------------------------------------------------------------------
PUSB_CLASS_DRIVER_DESCRIPTION 
USBD_FindClassDriver(
	IN PNP_CLASS_ID ClassId
	);

//------------------------------------------------
//  Needed in ISBD_Init were we initialize the
//  the DPC for timing when to start enumeration.
//------------------------------------------------
void    
USBD_DeviceEnumTimerProc(
    IN PKDPC Dpc,
    IN PVOID Unused1,
    IN PVOID Unused2,
    IN PVOID Unused3
    );

//------------------------------------------------
//  Entry Point XAPI must call
//------------------------------------------------
EXTERNUSB VOID USBD_Init(DWORD NumDeviceTypes, PXDEVICE_PREALLOC_TYPE DeviceTypes);

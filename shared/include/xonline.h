/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * xonline.h -- public API for Xbox Live (XOnline), the console's online
 * service layer. Titles include this header to sign users on, then reach the
 * back-end services: friends and presence, session matchmaking, competitions
 * and arbitration, statistics and leaderboards, messaging, teams, remote
 * storage, downloadable offerings and billing, player feedback, and the
 * string/query helpers.
 *
 * Almost every service call is asynchronous: the API kicks off work and hands
 * back an XONLINETASK_HANDLE that the title pumps to completion (see the Task
 * Pump section) before reading results. This header declares those entry
 * points along with the service IDs, task/HRESULT status codes, and the
 * request/result structures they exchange.
 */

#ifndef __XONLINE__
#define __XONLINE__

/*
 * xonline.h references winsock (IN_ADDR) and XNet (XNKID/XNADDR/XNKEY) types.
 * Pull in winsockx.h (itself include-guarded) so this header is self-sufficient
 * regardless of include order.
 */
#include <winsockx.h>

#ifdef __cplusplus
extern "C" {
#endif


#pragma pack(push, 8)

//
// XOnline Startup & Cleanup
//

// Parameters to XOnlineStartup. dwMaxPrivatePool caps, in bytes, the private
// heap the XOnline runtime carves out for itself; 0 selects the default.
typedef struct _XONLINE_STARTUP_PARAMS {

    DWORD dwMaxPrivatePool;

} XONLINE_STARTUP_PARAMS, *PXONLINE_STARTUP_PARAMS;

// Initialize the XOnline runtime. Must be called (once) before any other
// XOnline API. Requires that networking has already been brought up via XNet.
// pxosp may be NULL to accept the defaults.
XBOXAPI
HRESULT
WINAPI
XOnlineStartup(
    IN const XONLINE_STARTUP_PARAMS* pxosp);

// Tear down the XOnline runtime and release its resources. Pair with
// XOnlineStartup; do not call other XOnline APIs afterward.
XBOXAPI
HRESULT
WINAPI
XOnlineCleanup();


//
// XOnline Error Codes
//
// HRESULTs returned by XOnline APIs and, more often, surfaced as the completion
// status of an async task (via XOnlineTaskContinue or the matching *GetResults
// call). All use FACILITY_XONLINE; codes with the severity bit set (0x8015xxxx)
// are failures, the 0x0015xxxx variants are success/informational states. They
// are grouped below by the service that produces them.
//

#define FACILITY_XONLINE 21

// Generic Errors                                       = 0x80150XXX
#define XONLINE_E_OVERFLOW _HRESULT_TYPEDEF_(0x80150001L)
#define XONLINE_E_NO_SESSION _HRESULT_TYPEDEF_(0x80150002L)
#define XONLINE_E_USER_NOT_LOGGED_ON _HRESULT_TYPEDEF_(0x80150003L)
#define XONLINE_E_NO_GUEST_ACCESS _HRESULT_TYPEDEF_(0x80150004L)
#define XONLINE_E_NOT_INITIALIZED _HRESULT_TYPEDEF_(0x80150005L)
#define XONLINE_E_NO_USER _HRESULT_TYPEDEF_(0x80150006L)
#define XONLINE_E_INTERNAL_ERROR _HRESULT_TYPEDEF_(0x80150007L)
#define XONLINE_E_OUT_OF_MEMORY _HRESULT_TYPEDEF_(0x80150008L)
#define XONLINE_E_TASK_BUSY _HRESULT_TYPEDEF_(0x80150009L)
#define XONLINE_E_SERVER_ERROR _HRESULT_TYPEDEF_(0x8015000AL)
#define XONLINE_E_IO_ERROR _HRESULT_TYPEDEF_(0x8015000BL)
#define XONLINE_E_BAD_CONTENT_TYPE _HRESULT_TYPEDEF_(0x8015000CL)
#define XONLINE_E_USER_NOT_PRESENT _HRESULT_TYPEDEF_(0x8015000DL)
#define XONLINE_E_PROTOCOL_MISMATCH _HRESULT_TYPEDEF_(0x8015000EL)
#define XONLINE_E_INVALID_SERVICE_ID _HRESULT_TYPEDEF_(0x8015000FL)
#define XONLINE_E_INVALID_REQUEST _HRESULT_TYPEDEF_(0x80150010L)
#define XONLINE_E_TASK_THROTTLED _HRESULT_TYPEDEF_(0x80150011L)
#define XONLINE_E_TASK_ABORTED_BY_DUPLICATE _HRESULT_TYPEDEF_(0x80150012L)
#define XONLINE_E_INVALID_TITLE_ID _HRESULT_TYPEDEF_(0x80150013L)

// Failures from XOnlineLogon                           = 0x801510XX
#define XONLINE_E_LOGON_NO_NETWORK_CONNECTION _HRESULT_TYPEDEF_(0x80151000L)

// XOnlineLogon task successful return states
#define XONLINE_S_LOGON_CONNECTION_ESTABLISHED _HRESULT_TYPEDEF_(0x001510F0L)

// XOnlineLogon task failure return values
#define XONLINE_E_LOGON_CANNOT_ACCESS_SERVICE _HRESULT_TYPEDEF_(0x80151001L)
#define XONLINE_E_LOGON_UPDATE_REQUIRED _HRESULT_TYPEDEF_(0x80151002L)
#define XONLINE_E_LOGON_SERVERS_TOO_BUSY _HRESULT_TYPEDEF_(0x80151003L)
#define XONLINE_E_LOGON_CONNECTION_LOST _HRESULT_TYPEDEF_(0x80151004L)
#define XONLINE_E_LOGON_KICKED_BY_DUPLICATE_LOGON _HRESULT_TYPEDEF_(0x80151005L)
#define XONLINE_E_LOGON_INVALID_USER _HRESULT_TYPEDEF_(0x80151006L)

// Failures from XOnlineSilentLogon
#define XONLINE_E_SILENT_LOGON_DISABLED _HRESULT_TYPEDEF_(0x80151080L)
#define XONLINE_E_SILENT_LOGON_NO_ACCOUNTS _HRESULT_TYPEDEF_(0x80151081L)
#define XONLINE_E_SILENT_LOGON_PASSCODE_REQUIRED _HRESULT_TYPEDEF_(0x80151082L)

// Service errors after XOnlineLogon task completion    = 0x801511XX
#define XONLINE_E_LOGON_SERVICE_NOT_REQUESTED _HRESULT_TYPEDEF_(0x80151100L)
#define XONLINE_E_LOGON_SERVICE_NOT_AUTHORIZED _HRESULT_TYPEDEF_(0x80151101L)
#define XONLINE_E_LOGON_SERVICE_TEMPORARILY_UNAVAILABLE _HRESULT_TYPEDEF_(0x80151102L)

// User warnings after XOnlineLogon task completion     = 0x801512XX
#define XONLINE_S_LOGON_USER_HAS_MESSAGE _HRESULT_TYPEDEF_(0x001512F0L)

// User errors after XOnlineLogon task completion
#define XONLINE_E_LOGON_USER_ACCOUNT_REQUIRES_MANAGEMENT _HRESULT_TYPEDEF_(0x80151200L)

// XOnlineChangeLogonUsers task successful return states = 0x801513XX
#define XONLINE_S_LOGON_COMMIT_USER_CHANGE _HRESULT_TYPEDEF_(0x001513F0L)
#define XONLINE_S_LOGON_USER_CHANGE_COMPLETE _HRESULT_TYPEDEF_(0x001513F1L)

// XOnlineChangeLogonUsers task failure return values
#define XONLINE_E_LOGON_CHANGE_USER_FAILED _HRESULT_TYPEDEF_(0x80151300L)

// Other generic auth related errors                    = 0x801518XX
#define XONLINE_E_LOGON_MU_NOT_MOUNTED _HRESULT_TYPEDEF_(0x80151800L)
#define XONLINE_E_LOGON_MU_IO_ERROR _HRESULT_TYPEDEF_(0x80151801L)
#define XONLINE_E_LOGON_NOT_LOGGED_ON _HRESULT_TYPEDEF_(0x80151802L)


// Errors returned by Presence/Notification             = 0x801520XX
#define XONLINE_E_NOTIFICATION_SERVER_BUSY _HRESULT_TYPEDEF_(0x80152001L)
#define XONLINE_E_NOTIFICATION_LIST_FULL _HRESULT_TYPEDEF_(0x80152002L)
#define XONLINE_E_NOTIFICATION_BLOCKED _HRESULT_TYPEDEF_(0x80152003L)
#define XONLINE_E_NOTIFICATION_FRIEND_PENDING _HRESULT_TYPEDEF_(0x80152004L)
#define XONLINE_E_NOTIFICATION_FLUSH_TICKETS _HRESULT_TYPEDEF_(0x80152005L)
#define XONLINE_E_NOTIFICATION_TOO_MANY_REQUESTS _HRESULT_TYPEDEF_(0x80152006L)
#define XONLINE_E_NOTIFICATION_USER_ALREADY_EXISTS _HRESULT_TYPEDEF_(0x80152007L)
#define XONLINE_E_NOTIFICATION_USER_NOT_FOUND _HRESULT_TYPEDEF_(0x80152008L)
#define XONLINE_E_NOTIFICATION_OTHER_LIST_FULL _HRESULT_TYPEDEF_(0x80152009L)
#define XONLINE_E_NOTIFICATION_SELF _HRESULT_TYPEDEF_(0x8015200AL)
#define XONLINE_E_NOTIFICATION_SAME_TITLE _HRESULT_TYPEDEF_(0x8015200BL)
#define XONLINE_E_NOTIFICATION_NO_TASK _HRESULT_TYPEDEF_(0x8015200CL)

// Errors returned by teams                             = 0x801521XX
#define XONLINE_E_TEAMS_SERVER_BUSY _HRESULT_TYPEDEF_(0x80152100L)
#define XONLINE_E_TEAMS_TEAM_FULL _HRESULT_TYPEDEF_(0x80152101L)
#define XONLINE_E_TEAMS_MEMBER_PENDING _HRESULT_TYPEDEF_(0x80152102L)
#define XONLINE_E_TEAMS_TOO_MANY_REQUESTS _HRESULT_TYPEDEF_(0x80152103L)
#define XONLINE_E_TEAMS_USER_ALREADY_EXISTS _HRESULT_TYPEDEF_(0x80152104L)
#define XONLINE_E_TEAMS_USER_NOT_FOUND _HRESULT_TYPEDEF_(0x80152105L)
#define XONLINE_E_TEAMS_USER_TEAMS_FULL _HRESULT_TYPEDEF_(0x80152106L)
#define XONLINE_E_TEAMS_SELF _HRESULT_TYPEDEF_(0x80152107L)
#define XONLINE_E_TEAMS_NO_TASK _HRESULT_TYPEDEF_(0x80152108L)
#define XONLINE_E_TEAMS_TOO_MANY_TEAMS _HRESULT_TYPEDEF_(0x80152109L)
#define XONLINE_E_TEAMS_TEAM_ALREADY_EXISTS _HRESULT_TYPEDEF_(0x8015210AL)
#define XONLINE_E_TEAMS_TEAM_NOT_FOUND _HRESULT_TYPEDEF_(0x8015210BL)
#define XONLINE_E_TEAMS_INSUFFICIENT_PRIVILEGES _HRESULT_TYPEDEF_(0x8015210CL)
#define XONLINE_E_TEAMS_NAME_CONTAINS_BAD_WORDS _HRESULT_TYPEDEF_(0x8015210DL)
#define XONLINE_E_TEAMS_DESCRIPTION_CONTAINS_BAD_WORDS _HRESULT_TYPEDEF_(0x8015210EL)
#define XONLINE_E_TEAMS_MOTTO_CONTAINS_BAD_WORDS _HRESULT_TYPEDEF_(0x8015210FL)
#define XONLINE_E_TEAMS_URL_CONTAINS_BAD_WORDS _HRESULT_TYPEDEF_(0x80152110L)
#define XONLINE_E_TEAMS_NOT_A_MEMBER _HRESULT_TYPEDEF_(0x80152111L)
#define XONLINE_E_TEAMS_NO_ADMIN _HRESULT_TYPEDEF_(0x80152112L)

// Errors returned by offering service                  = 0x801530XX + 0x801531XX
#define XONLINE_S_OFFERING_NEW_CONTENT _HRESULT_TYPEDEF_(0x00153101L) // new content is available
#define XONLINE_S_OFFERING_NO_NEW_CONTENT _HRESULT_TYPEDEF_(0x00153102L) // no new content is available
#define XONLINE_E_OFFERING_BAD_REQUEST _HRESULT_TYPEDEF_(0x80153001L) // server received incorrectly formatted request
#define XONLINE_E_OFFERING_INVALID_USER _HRESULT_TYPEDEF_(0x80153002L) // cannot find account for this user
#define XONLINE_E_OFFERING_INVALID_OFFER_ID _HRESULT_TYPEDEF_(0x80153003L) // offer does not exist
#define XONLINE_E_OFFERING_INELIGIBLE_FOR_OFFER _HRESULT_TYPEDEF_(0x80153004L) // title not allowed to purchase this offer
#define XONLINE_E_OFFERING_OFFER_EXPIRED _HRESULT_TYPEDEF_(0x80153005L) // offer no longer available
#define XONLINE_E_OFFERING_SERVICE_UNREACHABLE _HRESULT_TYPEDEF_(0x80153006L) // apparent connectivity problems
#define XONLINE_E_OFFERING_PURCHASE_BLOCKED _HRESULT_TYPEDEF_(0x80153007L) // this user is not allowed to make purchases
#define XONLINE_E_OFFERING_PURCHASE_DENIED _HRESULT_TYPEDEF_(0x80153008L) // this user's payment is denied by billing provider
#define XONLINE_E_OFFERING_BILLING_SERVER_ERROR _HRESULT_TYPEDEF_(0x80153009L) // nonspecific billing provider error
#define XONLINE_E_OFFERING_OFFER_NOT_CANCELABLE _HRESULT_TYPEDEF_(0x8015300AL) // either this offer doesn't exist, or it's marked as un-cancelable
#define XONLINE_E_OFFERING_NOTHING_TO_CANCEL _HRESULT_TYPEDEF_(0x8015300BL) // this user doesn't have one of these anyways
#define XONLINE_E_OFFERING_ALREADY_OWN_MAX _HRESULT_TYPEDEF_(0x8015300CL) // this user already owns the maximum allowed
#define XONLINE_E_OFFERING_NO_CHARGE _HRESULT_TYPEDEF_(0x8015300DL) // this is a free offer; no purchase is necessary
#define XONLINE_E_OFFERING_PERMISSION_DENIED _HRESULT_TYPEDEF_(0x8015300EL) // permission denied
#define XONLINE_E_OFFERING_NAME_TAKEN _HRESULT_TYPEDEF_(0x8015300FL) // Name given to XOnlineVerifyNickname is taken (dosen't vet)

//  Errors returned by xcbk service                     = 0x801535XX

//  Errors returned by uacs service                     = 0x801540XX

// Errors returned by Notification                      = 0x801550XX
#define XONLINE_E_NOTIFICATION_BAD_CONTENT_TYPE _HRESULT_TYPEDEF_(0x80155000L)
#define XONLINE_E_NOTIFICATION_REQUEST_TOO_SMALL _HRESULT_TYPEDEF_(0x80155001L)
#define XONLINE_E_NOTIFICATION_INVALID_MESSAGE_TYPE _HRESULT_TYPEDEF_(0x80155002L)
#define XONLINE_E_NOTIFICATION_NO_ADDRESS _HRESULT_TYPEDEF_(0x80155003L)
#define XONLINE_E_NOTIFICATION_INVALID_PUID _HRESULT_TYPEDEF_(0x80155004L)
#define XONLINE_E_NOTIFICATION_NO_CONNECTION _HRESULT_TYPEDEF_(0x80155005L)
#define XONLINE_E_NOTIFICATION_SEND_FAILED _HRESULT_TYPEDEF_(0x80155006L)
#define XONLINE_E_NOTIFICATION_RECV_FAILED _HRESULT_TYPEDEF_(0x80155007L)
#define XONLINE_E_NOTIFICATION_MESSAGE_TRUNCATED _HRESULT_TYPEDEF_(0x80155008L)
#define XONLINE_E_NOTIFICATION_INVALID_TITLE_ID _HRESULT_TYPEDEF_(0x80155009L)

// Errors returned by Messages                          = 0x80155AXX
#define XONLINE_E_MESSAGE_INVALID_MESSAGE_ID _HRESULT_TYPEDEF_(0x80155A01L) // the specified message was not found
#define XONLINE_E_MESSAGE_PROPERTY_DOWNLOAD_REQUIRED _HRESULT_TYPEDEF_(0x80155A02L) // the property was too large to fit into the details block, it must be retrieved separately using XOnlineMessageDownloadAttachmentxxx
#define XONLINE_E_MESSAGE_PROPERTY_NOT_FOUND _HRESULT_TYPEDEF_(0x80155A03L) // the specified property tag was not found
#define XONLINE_E_MESSAGE_NO_VALID_SENDS_TO_REVOKE _HRESULT_TYPEDEF_(0x80155A04L) // no valid sends to revoke were found
#define XONLINE_E_MESSAGE_NO_MESSAGE_DETAILS _HRESULT_TYPEDEF_(0x80155A05L) // the specified message does not have any details
#define XONLINE_E_MESSAGE_INVALID_TITLE_ID _HRESULT_TYPEDEF_(0x80155A06L) // an invalid title ID was specified
#define XONLINE_E_MESSAGE_SENDER_BLOCKED _HRESULT_TYPEDEF_(0x80155A07L) // a send failed because the recipient has blocked the sender
#define XONLINE_E_MESSAGE_MAX_DETAILS_SIZE_EXCEEDED _HRESULT_TYPEDEF_(0x80155A08L) // the property couldn't be added because the maximum details size would be exceeded
#define XONLINE_E_MESSAGE_INVALID_MESSAGE_TYPE _HRESULT_TYPEDEF_(0x80155A09L)
#define XONLINE_E_MESSAGE_USER_OPTED_OUT _HRESULT_TYPEDEF_(0x80155A0AL) // a send failed because the message is marketing and the recipient has opted-out for the sending title
// Success codes returned by Messages                   = 0x00155AXX
#define XONLINE_S_MESSAGE_PENDING_SYNC _HRESULT_TYPEDEF_(0x00155A01L) // updated message list is currently being retrieved (after logon or disabling summary refresh), returned results may be out of date


//  Errors returned by matchmaking                      = 0x801551XX
#define XONLINE_E_MATCH_INVALID_SESSION_ID _HRESULT_TYPEDEF_(0x80155100L) // specified session id does not exist
#define XONLINE_E_MATCH_INVALID_TITLE_ID _HRESULT_TYPEDEF_(0x80155101L) // specified title id is zero, or does not exist
#define XONLINE_E_MATCH_INVALID_DATA_TYPE _HRESULT_TYPEDEF_(0x80155102L) // attribute ID or parameter type specifies an invalid data type
#define XONLINE_E_MATCH_REQUEST_TOO_SMALL _HRESULT_TYPEDEF_(0x80155103L) // the request did not meet the minimum length for a valid request
#define XONLINE_E_MATCH_REQUEST_TRUNCATED _HRESULT_TYPEDEF_(0x80155104L) // the self described length is greater than the actual buffer size
#define XONLINE_E_MATCH_INVALID_SEARCH_REQ _HRESULT_TYPEDEF_(0x80155105L) // the search request was invalid
#define XONLINE_E_MATCH_INVALID_OFFSET _HRESULT_TYPEDEF_(0x80155106L) // one of the attribute/parameter offsets in the request was invalid.  Will be followed by the zero based offset number.
#define XONLINE_E_MATCH_INVALID_ATTR_TYPE _HRESULT_TYPEDEF_(0x80155107L) // the attribute type was something other than user or session
#define XONLINE_E_MATCH_INVALID_VERSION _HRESULT_TYPEDEF_(0x80155108L) // bad protocol version in request
#define XONLINE_E_MATCH_OVERFLOW _HRESULT_TYPEDEF_(0x80155109L) // an attribute or parameter flowed past the end of the request
#define XONLINE_E_MATCH_INVALID_RESULT_COL _HRESULT_TYPEDEF_(0x8015510AL) // referenced stored procedure returned a column with an unsupported data type
#define XONLINE_E_MATCH_INVALID_STRING _HRESULT_TYPEDEF_(0x8015510BL) // string with length-prefix of zero, or string with no terminating null
#define XONLINE_E_MATCH_STRING_TOO_LONG _HRESULT_TYPEDEF_(0x8015510CL) // string exceeded 400 characters
#define XONLINE_E_MATCH_BLOB_TOO_LONG _HRESULT_TYPEDEF_(0x8015510DL) // blob exceeded 800 bytes
#define XONLINE_E_MATCH_INVALID_ATTRIBUTE_ID _HRESULT_TYPEDEF_(0x80155110L) // attribute id is invalid
#define XONLINE_E_MATCH_SESSION_ALREADY_EXISTS _HRESULT_TYPEDEF_(0x80155112L) // session id already exists in the db
#define XONLINE_E_MATCH_CRITICAL_DB_ERR _HRESULT_TYPEDEF_(0x80155115L) // critical error in db
#define XONLINE_E_MATCH_NOT_ENOUGH_COLUMNS _HRESULT_TYPEDEF_(0x80155116L) // search result set had too few columns
#define XONLINE_E_MATCH_PERMISSION_DENIED _HRESULT_TYPEDEF_(0x80155117L) // incorrect permissions set on search sp
#define XONLINE_E_MATCH_INVALID_PART_SCHEME _HRESULT_TYPEDEF_(0x80155118L) // title specified an invalid partitioning scheme
#define XONLINE_E_MATCH_INVALID_PARAM _HRESULT_TYPEDEF_(0x80155119L) // bad parameter passed to sp
#define XONLINE_E_MATCH_DATA_TYPE_MISMATCH _HRESULT_TYPEDEF_(0x8015511DL) // data type specified in attr id did not match type of attr being set
#define XONLINE_E_MATCH_SERVER_ERROR _HRESULT_TYPEDEF_(0x8015511EL) // error on server not correctable by client
#define XONLINE_E_MATCH_NO_USERS _HRESULT_TYPEDEF_(0x8015511FL) // no authenticated users in search request.
#define XONLINE_E_MATCH_INVALID_BLOB _HRESULT_TYPEDEF_(0x80155120L) // invalid blob attribute
#define XONLINE_E_MATCH_TOO_MANY_USERS _HRESULT_TYPEDEF_(0x80155121L) // too many users in search request
#define XONLINE_E_MATCH_INVALID_FLAGS _HRESULT_TYPEDEF_(0x80155122L) // invalid flags were specified in a search request

// Errors returned by uodb procs                        = 0x801560XX
#define XONLINE_E_UODB_KEY_ALREADY_EXISTS _HRESULT_TYPEDEF_(0x80156000L) // service key already exists when attempting to insert key

// Errors returned by Query service                     = 0x801561XX
#define XONLINE_E_QUERY_QUOTA_FULL _HRESULT_TYPEDEF_(0x80156101L) // this user or team's quota for the dataset is full.  you must remove an entity first.
#define XONLINE_E_QUERY_ENTITY_NOT_FOUND _HRESULT_TYPEDEF_(0x80156102L) // the requested entity didn't exist in the provided dataset.
#define XONLINE_E_QUERY_PERMISSION_DENIED _HRESULT_TYPEDEF_(0x80156103L) // the user tried to update or delete an entity that he didn't own.
#define XONLINE_E_QUERY_ATTRIBUTE_TOO_LONG _HRESULT_TYPEDEF_(0x80156104L) // attribute passed exceeds schema definition
#define XONLINE_E_QUERY_UNEXPECTED_ATTRIBUTE _HRESULT_TYPEDEF_(0x80156105L) // attribute passed was a bad param for the database operation
#define XONLINE_E_QUERY_INVALID_ACTION _HRESULT_TYPEDEF_(0x80156107L) // the specified action (or dataset) doesn't have a select action associated with it.
#define XONLINE_E_QUERY_SPEC_COUNT_MISMATCH _HRESULT_TYPEDEF_(0x80156108L) // the provided number of QUERY_ATTRIBUTE_SPECs doesn't match the number returned by the procedure
#define XONLINE_E_QUERY_DATASET_NOT_FOUND _HRESULT_TYPEDEF_(0x80156109L) // The specified dataset id was not found.
#define XONLINE_E_QUERY_PROCEDURE_NOT_FOUND _HRESULT_TYPEDEF_(0x8015610AL) // The specified proc index was not found.
#define XONLINE_E_QUERY_DUPLICATE_ENTRY _HRESULT_TYPEDEF_(0x8015610BL) // An entry already exists that conflicts with the unique data index specified for this dataset
#define XONLINE_E_QUERY_RETRY _HRESULT_TYPEDEF_(0x8015610CL) // An error occurred in the database requiring a retry


// Errors returned by Competitions service              = 0x801562XX
#define XONLINE_E_COMP_ACCESS_DENIED _HRESULT_TYPEDEF_(0x80156202L) // The specified source (client) is not permitted to execute this method
#define XONLINE_E_COMP_REGISTRATION_CLOSED _HRESULT_TYPEDEF_(0x80156203L) // The competition is closed to registration
#define XONLINE_E_COMP_FULL _HRESULT_TYPEDEF_(0x80156204L) // The competition has reached it's max enrollment
#define XONLINE_E_COMP_NOT_REGISTERED _HRESULT_TYPEDEF_(0x80156205L) // The user or team isn't registered for the competition
#define XONLINE_E_COMP_CANCELLED _HRESULT_TYPEDEF_(0x80156206L) // The competition has been cancelled, and the operation is invalid.
#define XONLINE_E_COMP_CHECKIN_TIME_INVALID _HRESULT_TYPEDEF_(0x80156207L) // The user is attempting to checkin to an event outside the allowed time.
#define XONLINE_E_COMP_CHECKIN_BAD_EVENT _HRESULT_TYPEDEF_(0x80156208L) // The user is attempting to checkin to an event in which they are not a valid participant.
#define XONLINE_E_COMP_EVENT_SCORED _HRESULT_TYPEDEF_(0x80156209L) // The user is attempting to checkin to an event which has already been scored by the service (user has forfeited or been ejected)
#define XONLINE_S_COMP_EVENT_SCORED _HRESULT_TYPEDEF_(0x00156209L) // The user is attempting to checkin to an event but the users event has been updated. Re-query for a new event
#define XONLINE_E_COMP_UNEXPECTED _HRESULT_TYPEDEF_(0x80156210L) // Results from the Database are unexpected or inconsistent with the current operation.
#define XONLINE_E_COMP_TOPOLOGY_ERROR _HRESULT_TYPEDEF_(0x80156216L) // The topology request cannot be fulfilled by the server
#define XONLINE_E_COMP_TOPOLOGY_PENDING _HRESULT_TYPEDEF_(0x80156217L) // The topology request has not completed yet
#define XONLINE_E_COMP_CHECKIN_TOO_EARLY _HRESULT_TYPEDEF_(0x80156218L) // The user is attempting to checkin to an event before the allowed time.
#define XONLINE_E_COMP_ALREADY_REGISTERED _HRESULT_TYPEDEF_(0x80156219L) // The user has already registered for this competition.
#define XONLINE_E_COMP_INVALID_ENTRANT_TYPE _HRESULT_TYPEDEF_(0x8015621AL) // dwTeamId was non-0 for a user competition, or dwTeamId was 0 for a team competition
#define XONLINE_E_COMP_TOO_LATE _HRESULT_TYPEDEF_(0x8015621BL) // The time alloted for performing the requested action has already passed.
#define XONLINE_E_COMP_TOO_EARLY _HRESULT_TYPEDEF_(0x8015621CL) // The specified action cannot yet be peformed .
#define XONLINE_E_COMP_NO_BYES_AVAILABLE _HRESULT_TYPEDEF_(0x8015621DL) // No byes remain to be granted
#define XONLINE_E_COMP_SERVICE_OUTAGE _HRESULT_TYPEDEF_(0x8015621EL) // A service outage has occured, try again in a bit

// Errors returned by the v1 Message Service            = 0x801570XX
#define XONLINE_E_MSGSVR_INVALID_REQUEST _HRESULT_TYPEDEF_(0x80157001L) // an invalid request type was received

// Errors returned by the String Service                = 0x801571XX
#define XONLINE_E_STRING_TOO_LONG _HRESULT_TYPEDEF_(0x80157101L) // the string was longer than the allowed maximum
#define XONLINE_E_STRING_OFFENSIVE_TEXT _HRESULT_TYPEDEF_(0x80157102L) // the string contains offensive text
#define XONLINE_E_STRING_NO_DEFAULT_STRING _HRESULT_TYPEDEF_(0x80157103L) // returned by AddString when no string of the language specified as the default is found
#define XONLINE_E_STRING_INVALID_LANGUAGE _HRESULT_TYPEDEF_(0x80157104L) // returned by AddString when an invalid language is specified for a string
#define XONLINE_E_STRING_LANGUAGE_DUPLICATE _HRESULT_TYPEDEF_(0x80157105L) // returned by AddString when a language is specified more than once in a single request

// Errors returned by the Feedback Service              = 0x801580XX
#define XONLINE_E_FEEDBACK_NULL_TARGET _HRESULT_TYPEDEF_(0x80158001L) // target PUID of feedback is NULL
#define XONLINE_E_FEEDBACK_BAD_TYPE _HRESULT_TYPEDEF_(0x80158002L) // bad feedback type
#define XONLINE_E_FEEDBACK_CANNOT_LOG _HRESULT_TYPEDEF_(0x80158006L) // cannot write to feedback log

// Errors returned by the Statistics Service            = 0x80159XXX
#define XONLINE_E_STAT_BAD_REQUEST _HRESULT_TYPEDEF_(0x80159001L) // server received incorrectly formatted request.
#define XONLINE_E_STAT_INVALID_TITLE_OR_LEADERBOARD _HRESULT_TYPEDEF_(0x80159002L) // title or leaderboard id were not recognized by the server.
#define XONLINE_E_STAT_TOO_MANY_SPECS _HRESULT_TYPEDEF_(0x80159004L) // too many stat specs in a request.
#define XONLINE_E_STAT_TOO_MANY_STATS _HRESULT_TYPEDEF_(0x80159005L) // too many stats in a spec or already stored for the user.
#define XONLINE_E_STAT_USER_NOT_FOUND _HRESULT_TYPEDEF_(0x80159003L) // user not found.
#define XONLINE_E_STAT_SET_FAILED_0 _HRESULT_TYPEDEF_(0x80159100L) // set operation failed on spec index 0
#define XONLINE_E_STAT_PERMISSION_DENIED _HRESULT_TYPEDEF_(0x80159200L) // operation failed because of credentials. UserId is not logged in or this operation is not supported in production (e.g. userId=0 in XOnlineStatReset)
#define XONLINE_E_STAT_LEADERBOARD_WAS_RESET _HRESULT_TYPEDEF_(0x80159201L) // operation failed because user was logged on before the leaderboard was reset.
#define XONLINE_E_STAT_INVALID_ATTACHMENT _HRESULT_TYPEDEF_(0x80159202L) // attachment is invalid.
#define XONLINE_S_STAT_CAN_UPLOAD_ATTACHMENT _HRESULT_TYPEDEF_(0x00159203L) // Use XOnlineStatWriteGetResults to get a handle to upload a attachment.
#define XONLINE_E_STAT_TOO_MANY_PARAMETERS _HRESULT_TYPEDEF_(0x80159204L)
#define XONLINE_E_STAT_TOO_MANY_PROCEDURES _HRESULT_TYPEDEF_(0x80159205L)
#define XONLINE_E_STAT_STAT_POST_PROC_ERROR _HRESULT_TYPEDEF_(0x80159206L)
#define XONLINE_E_STAT_NOT_ENOUGH_PARAMETERS _HRESULT_TYPEDEF_(0x80159208L)
#define XONLINE_E_STAT_INVALID_PROCEDURE _HRESULT_TYPEDEF_(0x80159209L)
#define XONLINE_E_STAT_EXCEEDED_WRITE_READ_LIMIT _HRESULT_TYPEDEF_(0x8015920aL)
#define XONLINE_E_STAT_LEADERBOARD_READONLY _HRESULT_TYPEDEF_(0x8015920bL)

//  Errors returned by xsuppapi service                 = 0x8015A0XX

// Errors returned by Signature Service                 = 0x8015B0XX
#define XONLINE_E_SIGNATURE_VER_INVALID_SIGNATURE _HRESULT_TYPEDEF_(0x8015B001L) // presented signature does not match
#define XONLINE_E_SIGNATURE_VER_UNKNOWN_KEY_VER _HRESULT_TYPEDEF_(0x8015B002L) // signature key version specified is not found among the valid signature keys
#define XONLINE_E_SIGNATURE_VER_UNKNOWN_SIGNATURE_VER _HRESULT_TYPEDEF_(0x8015B003L) // signature version is unknown, currently only version 1 is supported
#define XONLINE_E_SIGNATURE_BANNED_XBOX _HRESULT_TYPEDEF_(0x8015B004L) // signature is not calculated or revoked because Xbox is banned
#define XONLINE_E_SIGNATURE_BANNED_USER _HRESULT_TYPEDEF_(0x8015B005L) // signature is not calculated or revoked because at least one user is banned
#define XONLINE_E_SIGNATURE_BANNED_TITLE _HRESULT_TYPEDEF_(0x8015B006L) // signature is not calculated or revoked because the given title and version is banned
#define XONLINE_E_SIGNATURE_BANNED_DIGEST _HRESULT_TYPEDEF_(0x8015B007L) // signature is not calculated or revoked because the digest is banned
#define XONLINE_E_SIGNATURE_GET_BAD_AUTH_DATA _HRESULT_TYPEDEF_(0x8015B008L) // fail to retrieve AuthData from SG, returned by GetSigningKey api
#define XONLINE_E_SIGNATURE_SERVICE_UNAVAILABLE _HRESULT_TYPEDEF_(0x8015B009L) // fail to retrieve a signature server master key, returned by GetSigningKey or SignOnBehalf api

// Errors returned by Arbitration Service                          = 0x8015B1XX
#define XONLINE_E_ARBITRATION_SERVICE_UNAVAILABLE _HRESULT_TYPEDEF_(0x8015B101L) // Service temporarily unavailable
#define XONLINE_E_ARBITRATION_INVALID_REQUEST _HRESULT_TYPEDEF_(0x8015B102L) // The request is invalidly formatted
#define XONLINE_E_ARBITRATION_SESSION_NOT_FOUND _HRESULT_TYPEDEF_(0x8015B103L) // The session is not found or has expired
#define XONLINE_E_ARBITRATION_REGISTRATION_FLAGS_MISMATCH _HRESULT_TYPEDEF_(0x8015B104L) // The session was registered with different flags by another Xbox
#define XONLINE_E_ARBITRATION_REGISTRATION_SESSION_TIME_MISMATCH _HRESULT_TYPEDEF_(0x8015B105L) // The session was registered with a different session time by another Xbox
#define XONLINE_E_ARBITRATION_REGISTRATION_TOO_LATE _HRESULT_TYPEDEF_(0x8015B106L) // Registration came too late, the session has already been arbitrated
#define XONLINE_E_ARBITRATION_NEED_TO_REGISTER_FIRST _HRESULT_TYPEDEF_(0x8015B107L) // Must register in seesion first, before any other activity
#define XONLINE_E_ARBITRATION_TIME_EXTENSION_NOT_ALLOWED _HRESULT_TYPEDEF_(0x8015B108L) // Time extension of this session not allowed, or session is already arbitrated
#define XONLINE_E_ARBITRATION_INCONSISTENT_FLAGS _HRESULT_TYPEDEF_(0x8015B109L) // Inconsistent flags are used in the request
#define XONLINE_E_ARBITRATION_INCONSISTENT_COMPETITION_STATUS _HRESULT_TYPEDEF_(0x8015B10AL) // Whether the session is a competition is inconsistent between registration and report
#define XONLINE_E_ARBITRATION_REPORT_ALREADY_CALLED _HRESULT_TYPEDEF_(0x8015b10BL) // Report call for this session already made by this client
#define XONLINE_E_ARBITRATION_TOO_MANY_XBOXES_IN_SESSION _HRESULT_TYPEDEF_(0x8015b10CL) // Only up to 255 Xboxes can register in a session
#define XONLINE_E_ARBITRATION_1_XBOX_1_USER_SESSION_NOT_ALLOWED _HRESULT_TYPEDEF_(0x8015b10DL) // Single Xbox single user sessions should not be arbitrated
#define XONLINE_E_ARBITRATION_REPORT_TOO_LARGE _HRESULT_TYPEDEF_(0x8015b10EL) // The stats or query submission is too large
#define XONLINE_E_ARBITRATION_INVALID_TEAMTICKET _HRESULT_TYPEDEF_(0x8015b10FL) // An invalid team ticket was submitted
// Arbitration success HRESULTS
#define XONLINE_S_ARBITRATION_INVALID_XBOX_SPECIFIED _HRESULT_TYPEDEF_(0x0015b1F0L) // Invalid/duplicate Xbox specified in lost connectivity or suspicious info. Never the less, this report is accepted
#define XONLINE_S_ARBITRATION_INVALID_USER_SPECIFIED _HRESULT_TYPEDEF_(0x0015b1F1L) // Invalid/duplicate user specified in lost connectivity or suspicious info. Never the less, this report is accepted
#define XONLINE_S_ARBITRATION_DIFFERENT_RESULTS_DETECTED _HRESULT_TYPEDEF_(0x0015b1F2L) // Differing result submissions have been detected in this session. Never the less, this report submission is accepted

// Errors returned by the Storage services              = 0x8015C0XX
#define XONLINE_E_STORAGE_INVALID_REQUEST _HRESULT_TYPEDEF_(0x8015c001L) // Request is invalid
#define XONLINE_E_STORAGE_ACCESS_DENIED _HRESULT_TYPEDEF_(0x8015c002L) // Client doesn't have the rights to upload the file
#define XONLINE_E_STORAGE_FILE_IS_TOO_BIG _HRESULT_TYPEDEF_(0x8015c003L) // File is too big
#define XONLINE_E_STORAGE_FILE_NOT_FOUND _HRESULT_TYPEDEF_(0x8015c004L) // File not found
#define XONLINE_E_STORAGE_INVALID_ACCESS_TOKEN _HRESULT_TYPEDEF_(0x8015c005L) // Access token signature is invalid
#define XONLINE_E_STORAGE_CANNOT_FIND_PATH _HRESULT_TYPEDEF_(0x8015c006L) // name resolution failed
#define XONLINE_E_STORAGE_FILE_IS_ELSEWHERE _HRESULT_TYPEDEF_(0x8015c007L) // redirection request
#define XONLINE_E_STORAGE_INVALID_STORAGE_PATH _HRESULT_TYPEDEF_(0x8015c008L) // Invalid storage path
#define XONLINE_E_STORAGE_INVALID_FACILITY _HRESULT_TYPEDEF_(0x8015c009L) // Invalid facility code
#define XONLINE_E_STORAGE_UNKNOWN_DOMAIN _HRESULT_TYPEDEF_(0x8015c00AL) // Bad pathname
#define XONLINE_E_STORAGE_SYNC_TIME_SKEW _HRESULT_TYPEDEF_(0x8015c00BL) // SyncDomain timestamp skew
#define XONLINE_E_STORAGE_SYNC_TIME_SKEW_LOCALTIME _HRESULT_TYPEDEF_(0x8015c00CL) // SyncDomain timestamp appears to be localtime
#define XONLINE_E_STORAGE_QUOTA_EXCEEDED _HRESULT_TYPEDEF_(0x8015c00DL) // Quota exceeded for storage domain
#define XONLINE_E_STORAGE_UNSUPPORTED_CONTENT_TYPE _HRESULT_TYPEDEF_(0x8015c00EL) // The type of the content is not supported by this API
#define XONLINE_E_STORAGE_FILE_ALREADY_EXISTS _HRESULT_TYPEDEF_(0x8015c011L) // File already exists and storage domain does not allow overwrites
#define XONLINE_E_STORAGE_DATABASE_ERROR _HRESULT_TYPEDEF_(0x8015c012L) // Unknown database error
#define XONLINE_S_STORAGE_FILE_NOT_MODIFIED _HRESULT_TYPEDEF_(0x0015c013L) // The file was not modified since the last installation

// Errors returned by billing services                      = 0x80162XXX - 0x8016EXXX
#define XONLINE_E_BILLING_AUTHORIZATION_FAILED _HRESULT_TYPEDEF_(0x80167611) // Credit card authorization failed; user should update credit card info in Dash.
#define XONLINE_E_BILLING_CREDIT_CARD_EXPIRED _HRESULT_TYPEDEF_(0x80167531) // The credit card has expired or will expire this month; user should update card info in Dash.
#define XONLINE_E_BILLING_NON_ACTIVE_ACCOUNT _HRESULT_TYPEDEF_(0x80169d94) // The account specified is no longer active; user should call customer service
#define XONLINE_E_BILLING_INVALID_PAYMENT_INSTRUMENT_STATUS _HRESULT_TYPEDEF_(0x80169e7f) // User's payment instrument is in a bad state. They should call customer service to rectify the issue.


//
// XOnline Task Pump
//
// The async model behind nearly every XOnline call. A service API that takes an
// OUT PXONLINETASK_HANDLE starts work on the network and returns immediately
// with a handle. The title drives that handle forward by calling
// XOnlineTaskContinue repeatedly (typically once per frame): while the task is
// still working it returns XONLINETASK_S_RUNNING, and when finished it returns
// the task's terminal HRESULT -- SUCCEEDED for completion, or an XONLINE_E_*
// failure. Once complete, the matching XOnline...GetResults function copies the
// results out. Always XOnlineTaskClose the handle when done, success or not.
//
// The optional hWorkEvent supplied at task creation is signaled when the task
// has new work to pump, letting the title wait on it instead of polling.
//

DECLARE_HANDLE(XONLINETASK_HANDLE);
typedef XONLINETASK_HANDLE* PXONLINETASK_HANDLE;

// Return states from XOnlineTaskContinue. S_RUNNING means keep pumping;
// S_RESULTS_AVAIL signals partial results are ready to read while the task
// continues; S_RUNNING_IDLE means running but currently blocked on external
// input; S_SUCCESS is normal completion.
#define XONLINETASK_S_RUNNING (S_OK)
#define XONLINETASK_S_SUCCESS _HRESULT_TYPEDEF_(0x001500F0L)
#define XONLINETASK_S_RESULTS_AVAIL _HRESULT_TYPEDEF_(0x001500F1L)
#define XONLINETASK_S_RUNNING_IDLE _HRESULT_TYPEDEF_(0x001500F2L)


// Advance an outstanding task. Call in a loop until it returns other than
// XONLINETASK_S_RUNNING; the returned HRESULT is then the task's final status.
XBOXAPI
HRESULT
WINAPI
XOnlineTaskContinue(
    IN XONLINETASK_HANDLE hTask);

// Release a task handle and its resources. Valid at any point; cancels the task
// if it is still running.
XBOXAPI
HRESULT
WINAPI
XOnlineTaskClose(
    IN XONLINETASK_HANDLE hTask);


//
// XOnline Authentication
//

// Gamertag buffer is 16 bytes including the terminating null (15 usable
// characters). Up to 4 users can be signed on at once; a passcode is 4 button
// presses.
#define XONLINE_GAMERTAG_SIZE 16
#define XONLINE_MAX_GAMERTAG_LENGTH (XONLINE_GAMERTAG_SIZE - 1)
#define XONLINE_PASSCODE_LENGTH 4
#define XONLINE_MAX_LOGON_USERS 4

// A passcode digit is one controller input; a passcode is XONLINE_PASSCODE_LENGTH
// of these packed one per byte in XONLINE_USER.passcode.
typedef enum {
    XONLINE_PASSCODE_DPAD_UP = 1,
    XONLINE_PASSCODE_DPAD_DOWN,
    XONLINE_PASSCODE_DPAD_LEFT,
    XONLINE_PASSCODE_DPAD_RIGHT,
    XONLINE_PASSCODE_GAMEPAD_X,
    XONLINE_PASSCODE_GAMEPAD_Y,
    XONLINE_PASSCODE_GAMEPAD_LEFT_TRIGGER = 9,
    XONLINE_PASSCODE_GAMEPAD_RIGHT_TRIGGER
} XONLINE_PASSCODE_TYPE;

// Classification of the console's NAT, as detected against the Live servers and
// returned by XOnlineGetNatType. OPEN peers can host and connect freely; STRICT
// peers may only reach OPEN peers, which affects matchmaking eligibility.
typedef enum {
    XONLINE_NAT_OPEN = 1,
    XONLINE_NAT_MODERATE,
    XONLINE_NAT_STRICT
} XONLINE_NAT_TYPE;


// Service IDs passed in the pdwServiceIDs array to XOnlineLogon/XOnlineSilentLogon
// (and to XOnlineGetServiceInfo/XOnlineThrottle*) to request access to a Live
// back-end service. Only the services a title logs on for are usable.
#define XONLINE_STRING_SERVICE ((DWORD)2)
#define XONLINE_CONTENT_AVAILABLE_SERVICE ((DWORD)4)
#define XONLINE_MATCHMAKING_SERVICE ((DWORD)6)
#define XONLINE_STATISTICS_SERVICE ((DWORD)7)
#define XONLINE_FEEDBACK_SERVICE ((DWORD)8)
#define XONLINE_BILLING_OFFERING_SERVICE ((DWORD)9)
#define XONLINE_NICKNAME_VERIFICATION_SERVICE ((DWORD)9)
#define XONLINE_SIGNATURE_SERVICE ((DWORD)12)
#define XONLINE_QUERY_SERVICE ((DWORD)13)
#define XONLINE_STORAGE_SERVICE ((DWORD)15)
#define XONLINE_ARBITRATION_SERVICE ((DWORD)16)
#define XONLINE_MESSAGING_SERVICE ((DWORD)18)
#define XONLINE_TEAM_SERVICE ((DWORD)19)
#define XONLINE_NAT_TYPE_DETECTION_SERVICE ((DWORD)20)

#define XONLINE_INVALID_SERVICE ((DWORD)0)


// Bitfields packed into XUID.dwUserFlags: guest slot number, the user's
// no-show/disconnect reputation ratings, country ID, and per-user parental
// permission bits (voice, purchase, nickname, shared content). Decode them with
// the XOnline...  accessor macros below rather than masking by hand.
#define XONLINE_USER_GUEST_MASK 0x00000003
#define XONLINE_USER_NOSHOW_RATING_MASK 0x0000001C
#define XONLINE_USER_DISCONNECT_RATING_MASK 0x000000E0

#define XONLINE_USER_COUNTRY_MASK 0x0000ff00

#define XONLINE_USER_VOICE_NOT_ALLOWED 0x00010000
#define XONLINE_USER_PURCHASE_NOT_ALLOWED 0x00020000
#define XONLINE_USER_NICKNAME_NOT_ALLOWED 0x00040000
#define XONLINE_USER_SHARED_CONTENT_NOT_ALLOWED 0x00080000

// Accessors for the dwUserFlags bitfields above. The IsUser... macros return
// TRUE when the corresponding privilege is permitted; guest 0 denotes the
// primary (non-guest) account.
#define XOnlineUserCountryId(dwUserFlags) ((BYTE)(((dwUserFlags) & XONLINE_USER_COUNTRY_MASK) >> 8))

#define XOnlineIsUserVoiceAllowed(dwUserFlags) (((dwUserFlags) & XONLINE_USER_VOICE_NOT_ALLOWED) == 0)

#define XOnlineIsUserPurchaseAllowed(dwUserFlags) (((dwUserFlags) & XONLINE_USER_PURCHASE_NOT_ALLOWED) == 0)

#define XOnlineIsUserNicknameAllowed(dwUserFlags) (((dwUserFlags) & XONLINE_USER_NICKNAME_NOT_ALLOWED) == 0)

#define XOnlineIsUserSharedContentAllowed(dwUserFlags) (((dwUserFlags) & XONLINE_USER_SHARED_CONTENT_NOT_ALLOWED) == 0)

#define XOnlineUserNoShowRating(dwUserFlags) (((dwUserFlags) & XONLINE_USER_NOSHOW_RATING_MASK) >> 2)

#define XOnlineUserDisconnectRating(dwUserFlags) (((dwUserFlags) & XONLINE_USER_DISCONNECT_RATING_MASK) >> 5)

#define XOnlineIsUserGuest(dwUserFlags) (((dwUserFlags) & XONLINE_USER_GUEST_MASK) != 0)

#define XOnlineUserGuestNumber(dwUserFlags) ((dwUserFlags) & XONLINE_USER_GUEST_MASK)

#define XOnlineSetUserGuestNumber(dwUserFlags, guestNumber) ((dwUserFlags) = ((dwUserFlags) & ~XONLINE_USER_GUEST_MASK) | (guestNumber & XONLINE_USER_GUEST_MASK))


// The persistent Live identity of a user or a team: a 64-bit ID plus the packed
// dwUserFlags described above. The same union holds a team ID when the XUID
// names a team (test with XOnlineXUIDIsTeam). This is the currency of every
// friends/presence/messaging/team API.
#pragma pack(push, 4)

typedef struct _XUID {
    union {
        ULONGLONG qwUserID;
        ULONGLONG qwTeamID;
    };
    DWORD dwUserFlags;
} XUID;

#pragma pack(pop)

// TRUE when two XUIDs name the same user account and guest slot. Compare with
// this rather than memcmp, since dwUserFlags carries fields beyond identity.
#define XOnlineAreUsersIdentical(pXUID1, pXUID2) (((pXUID1)->qwUserID == (pXUID2)->qwUserID) && \
    (XOnlineUserGuestNumber((pXUID1)->dwUserFlags) == XOnlineUserGuestNumber((pXUID2)->dwUserFlags)))

// TRUE when the XUID names a team rather than a user (team IDs carry a 0xFE tag
// in their top byte).
#define XOnlineXUIDIsTeam(pxuid) (((pxuid)->qwUserID & 0xFF00000000000000) == 0xFE00000000000000)


// Opaque 64-bit identifier for a purchasable/downloadable offering.
typedef ULONGLONG XOFFERING_ID;


#pragma pack(push, 4)

// dwUserOptions flags on a stored user account. REQUIRE_PASSCODE means the
// account is passcode-protected; the CAME_FROM_MU / MU_PORT / MU_SLOT bits
// record which memory unit the account was loaded from.
#define XONLINE_USER_RESERVED_SIZE 72
#define XONLINE_MAX_STORED_ONLINE_USERS 16


#define XONLINE_USER_OPTION_REQUIRE_PASSCODE 0x00000001
#define XONLINE_USER_OPTION_CAME_FROM_MU 0x80000000
#define XONLINE_USER_OPTION_MU_PORT_MASK 0x60000000
#define XONLINE_USER_OPTION_MU_PORT_SHIFT 29
#define XONLINE_USER_OPTION_MU_SLOT_MASK 0x10000000
#define XONLINE_USER_OPTION_MU_SLOT_SHIFT 28

// A stored user account as returned by XOnlineGetUsers and passed to
// XOnlineLogon. Fill in passcode only for accounts that require one; after a
// logon, hr carries the per-user sign-on result.
typedef struct _XONLINE_USER {
    XUID xuid;
    CHAR szGamertag[XONLINE_GAMERTAG_SIZE];
    DWORD dwUserOptions;
    BYTE passcode[XONLINE_PASSCODE_LENGTH];
    BYTE reserved[XONLINE_USER_RESERVED_SIZE];
    HRESULT hr;
} XONLINE_USER, *PXONLINE_USER;

// Address of a Live service endpoint, as returned by XOnlineGetServiceInfo for
// a service the title logged on for.
typedef struct _XONLINE_SERVICE_INFO {
    DWORD dwServiceID;
    IN_ADDR serviceIP;
    WORD wServicePort;
    WORD wReserved;
} XONLINE_SERVICE_INFO, *PXONLINE_SERVICE_INFO;

#pragma pack(pop)


// Enumerate the user accounts stored on the console's hard disk and mounted
// memory units. On entry *pdwUsers is the pUsers capacity; on exit it is the
// count filled in. Use these XONLINE_USERs as the input to XOnlineLogon.
XBOXAPI
HRESULT
WINAPI
XOnlineGetUsers(
    OUT PXONLINE_USER pUsers,
    OUT DWORD* pdwUsers);

// Begin signing the given users on to Live and requesting the listed services.
// Asynchronous: pump the returned task, then call XOnlineLogonTaskGetResults.
// The task stays alive after logon as the connection manager; keep pumping it
// for the duration of the online session and watch for connection-lost status.
XBOXAPI
HRESULT
WINAPI
XOnlineLogon(
    IN const XONLINE_USER* pUsers,
    IN const DWORD* pdwServiceIDs,
    IN DWORD dwServices,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE pHandle);

// Poll the logon task for its current connection state; returns
// XONLINE_S_LOGON_CONNECTION_ESTABLISHED once connected, an XONLINE_E_LOGON_*
// failure, or XONLINETASK_S_RUNNING while still connecting.
XBOXAPI
HRESULT
WINAPI
XOnlineLogonTaskGetResults(
    IN XONLINETASK_HANDLE hLogonTask);


// Change the set of signed-on users without a full re-logon (e.g. add or drop a
// controller's user). Async; results via XOnlineChangeLogonUsersTaskGetResults.
XBOXAPI
HRESULT
WINAPI
XOnlineChangeLogonUsers(
    IN const XONLINE_USER* pUsers,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE pHandle);

// Retrieve the outcome of an XOnlineChangeLogonUsers task; *phr receives the
// commit/complete or failure HRESULT for the user change.
XBOXAPI
HRESULT
WINAPI
XOnlineChangeLogonUsersTaskGetResults(
    IN XONLINETASK_HANDLE hLogonTask,
    OUT HRESULT* phr);

// Pointer to the runtime's array of XONLINE_MAX_LOGON_USERS currently signed-on
// users (indexed by user index). Slots for absent users have a zeroed xuid.
XBOXAPI
PXONLINE_USER
WINAPI
XOnlineGetLogonUsers();

// The console's detected NAT classification (see XONLINE_NAT_TYPE).
XBOXAPI
XONLINE_NAT_TYPE
WINAPI
XOnlineGetNatType();

// Look up the endpoint for one requested service after logon; fills pServiceInfo.
XBOXAPI
HRESULT
WINAPI
XOnlineGetServiceInfo(
    IN DWORD dwServiceID,
    OUT PXONLINE_SERVICE_INFO pServiceInfo);

// Like XOnlineLogon but signs on the users already cached from a prior session
// without prompting, using no XONLINE_USER array. Fails with
// XONLINE_E_SILENT_LOGON_* when silent logon is disabled, no accounts are
// cached, or a passcode is required.
XBOXAPI
HRESULT
WINAPI
XOnlineSilentLogon(
    IN const DWORD* pdwServiceIDs,
    IN DWORD dwServices,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE pHandle);

//
// Preserving state across reboots
//
// A title that reboots into another XBE (e.g. to launch the downloader) can
// carry its Live session forward: XOnlineSaveLogonState serializes the current
// logon into an opaque XONLINE_LOGON_STATE blob to hand across in launch data,
// and the next title reconstructs the users and services from it with
// XOnlineRetrieveLogonState before re-logging on.
//

// This should be same as XONLINE_MAX_NUMBER_SERVICE
#define XONLINE_MAX_LOGON_STATE_SERVICES 16
#define XONLINE_LOGON_STATE_SIZE (XONLINE_MAX_LOGON_USERS * sizeof(XONLINE_USER) + XONLINE_MAX_LOGON_STATE_SERVICES * sizeof(DWORD))

#define XONLINE_LOGON_STATE_TYPE 0x4C
#define XONLINE_LOGON_STATE_VERSION 1

// Opaque, versioned snapshot of the logged-on users and requested services.
// Treat Data as private to the XOnline runtime; move the whole struct intact.
typedef struct _XONLINE_LOGON_STATE {
    BYTE bType;
    BYTE bVersion;
    WORD cbSize;
    BYTE Data[XONLINE_LOGON_STATE_SIZE];
} XONLINE_LOGON_STATE, *PXONLINE_LOGON_STATE;

//
// Launch data passed to Downloader.XBE
//

// Launch-data layout for rebooting into the system downloader title. dwID is
// LAUNCH_DATA_DOWNLOADER_ID, LogonState carries the saved session, and
// UserDefined is title space returned when the downloader reboots back.
#define LAUNCH_DATA_DOWNLOADER_ID 'dl01'

typedef struct _LD_DOWNLOADER {
    DWORD dwID;
    DWORD dwBitFilter;
    BYTE bPremiumLogonPort;
    BYTE Reserved1[3];
    DWORD Reserved2;
    XONLINE_LOGON_STATE LogonState;
    BYTE UserDefined[MAX_LAUNCH_DATA_SIZE - (20 + XONLINE_LOGON_STATE_SIZE)];
} LD_DOWNLOADER, *PLD_DOWNLOADER;


// Serialize the current logon into pLogonState for transfer across a reboot.
XBOXAPI
HRESULT
WINAPI
XOnlineSaveLogonState(
    OUT PXONLINE_LOGON_STATE pLogonState);

// Reconstruct the users and service IDs from a saved logon state. On entry
// *pdwServices is the pdwServiceIDs capacity; on exit it is the count restored.
// Feed the recovered users/services back into XOnlineLogon to reconnect.
XBOXAPI
HRESULT
WINAPI
XOnlineRetrieveLogonState(
    IN const XONLINE_LOGON_STATE* pLogonState,
    OUT PXONLINE_USER pUsers,
    OUT DWORD* pdwServiceIDs,
    IN OUT DWORD* pdwServices);


//
// XOnline Title Update (Security updates)
//

// Reboot into the system downloader to fetch and apply a mandatory security
// update for this title. dwContext is passed through to the update flow.
// See XOnlineTitleUpdateEx for the launch-data variant.
XBOXAPI
HRESULT
WINAPI
XOnlineTitleUpdate(
    IN DWORD dwContext);

//
// Signature Service APIs
//
// The signature service lets a title verify that a digest (e.g. of stats or a
// game result) was signed by Live, guarding against tampered peer submissions.
//

// One (digest, online-signature) pair to check. pbDigest is the data hash;
// pbOnlineSignature is the Live-issued signature over it.
typedef struct
{
    DWORD cbDigest;
    PBYTE pbDigest;
    DWORD cbOnlineSignature;
    PBYTE pbOnlineSignature;

} XONLINE_SIGNATURE_TO_VERIFY;

typedef XONLINE_SIGNATURE_TO_VERIFY* PXONLINE_SIGNATURE_TO_VERIFY;

// Verify a batch of signatures against the service. Async; the per-signature
// results are collected by XOnlineSignatureVerifyGetResults.
XBOXAPI
HRESULT
WINAPI
XOnlineSignatureVerify(
    IN const XONLINE_SIGNATURE_TO_VERIFY* rgSignaturesToVerify,
    IN DWORD dwSignaturesToVerify,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Retrieve the array of per-signature HRESULTs from a completed verify task;
// *pdwHresults receives the count, *prgHresults points at runtime-owned storage.
XBOXAPI
HRESULT
WINAPI
XOnlineSignatureVerifyGetResults(
    IN XONLINETASK_HANDLE hTask,
    OUT HRESULT** prgHresults,
    OUT DWORD* pdwHresults);

//
// XOnline Storage
//
// Server-side file storage for titles: upload/download packages and blobs to
// per-title, per-user, or service-owned areas, keyed by a facility plus a
// storage path. All transfers are async tasks; long ones expose progress via
// XOnlineStorageGetProgress. Paths are wide strings under XONLINESTORAGE_MAX_PATH.
//

// Upload a file identified by a server file reference (as returned by a stats
// write) into szDirectory. Async.
XBOXAPI
HRESULT
WINAPI
XOnlineStorageUpload(
    IN HANDLE hServerFileReference,
    IN LPCSTR szDirectory,
    IN DWORD dwUploadFlags,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Storage facility, selecting which namespace/quota a file lives in. STATS,
// MESSAGING and TEAMS are service-owned; PER_TITLE and PER_USER_TITLE are the
// title's own areas.
typedef enum {
    XONLINESTORAGE_FACILITY_INVALID = 0,
    XONLINESTORAGE_FACILITY_STATS,
    XONLINESTORAGE_FACILITY_MESSAGING,
    XONLINESTORAGE_FACILITY_TEAMS,
    XONLINESTORAGE_FACILITY_PER_TITLE,
    XONLINESTORAGE_FACILITY_PER_USER_TITLE,
    XONLINESTORAGE_FACILITY_MAX

} XONLINESTORAGE_FACILITY;

#define XONLINESTORAGE_MAX_PATH 256

// Compose the canonical server path for a stored file from its facility, owner
// (user and/or team) and file name; the path then addresses the other storage
// APIs. *pcchStorageServerPath is in/out (capacity then length).
XBOXAPI
HRESULT
WINAPI
XOnlineStorageCreateServerPath(
    IN DWORD dwFacility,
    IN ULONGLONG qwUserID,
    IN ULONGLONG qwTeamID,
    IN LPCWSTR wszStorageFileName,
    OUT LPWSTR wszStorageServerPath,
    IN OUT DWORD* pcchStorageServerPath);

// Upload the contents of a local directory as a named storage file, expiring on
// the server at ftServerExpirationDate. Async.
XBOXAPI
HRESULT
WINAPI
XOnlineStorageUploadByServerPath(
    IN DWORD dwFacility,
    IN DWORD dwUserIndex,
    IN LPCWSTR wszStorageFileName,
    IN FILETIME ftServerExpirationDate,
    IN LPCSTR szDirectory,
    IN DWORD dwUploadFlags,
    IN HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Download a stored file and install it into szInstallDirectory. Async; use
// XOnlineStorageGetProgress to track a large download.
XBOXAPI
HRESULT
WINAPI
XOnlineStorageDownload(
    IN DWORD dwFacility,
    IN DWORD dwUserIndex,
    IN LPCWSTR wszStoragePath,
    IN LPCSTR szInstallDirectory,
    IN DWORD dwDownloadFlags,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Upload a blob straight from memory as a named storage file. Async.
XBOXAPI
HRESULT
WINAPI
XOnlineStorageUploadFromMemory(
    IN DWORD dwFacility,
    IN DWORD dwUserIndex,
    IN LPCWSTR wszStorageFileName,
    IN FILETIME ftServerExpirationDate,
    IN PBYTE pbDataToUpload,
    IN DWORD cbDataToUpload,
    IN DWORD dwUploadFlags,
    IN HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Download a stored file into a caller-supplied memory buffer. Async; retrieve
// the received bytes and metadata with XOnlineStorageDownloadToMemoryGetResults.
XBOXAPI
HRESULT
WINAPI
XOnlineStorageDownloadToMemory(
    IN DWORD dwFacility,
    IN DWORD dwUserIndex,
    IN LPCWSTR wszStoragePath,
    IN PBYTE pbReceiveBuffer,
    IN DWORD cbReceiveBuffer,
    IN DWORD dwDownloadFlags,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Results of a download-to-memory task: received data pointer/size, total size,
// owning user PUID, and creation date. All OUT parameters are optional.
XBOXAPI
HRESULT
WINAPI
XOnlineStorageDownloadToMemoryGetResults(
    IN XONLINETASK_HANDLE hTask,
    OUT OPTIONAL PBYTE* ppbReceivedData,
    OUT OPTIONAL DWORD* pcbReceivedData,
    OUT OPTIONAL DWORD* pcbDataTotal,
    OUT OPTIONAL ULONGLONG* pqwOwnerPuid,
    OUT OPTIONAL FILETIME* pftCreationDate);

// Delete a stored file. Async.
XBOXAPI
HRESULT
WINAPI
XOnlineStorageDeleteFile(
    IN DWORD dwFacility,
    IN DWORD dwUserIndex,
    IN LPCWSTR wszStorageFileName,
    IN HANDLE hWorkEvent,
    OUT XONLINETASK_HANDLE* phTask);

// List stored files under an enumeration path, a page at a time from
// dwStartingIndex. Async; XOnlineStorageEnumerateGetResults returns the
// XONLINESTORAGE_FILE_INFO records.
XBOXAPI
HRESULT
WINAPI
XOnlineStorageEnumerate(
    IN DWORD dwFacility,
    IN DWORD dwUserIndex,
    IN LPCWSTR wszStorageEnumerationPath,
    IN DWORD dwStartingIndex,
    IN DWORD cMaxResultsToReturn,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Storage content kind: an installable PACKAGE versus an opaque BLOB.
typedef enum {
    XONLINESTORAGE_CONTENT_TYPE_PACKAGE = 0,
    XONLINESTORAGE_CONTENT_TYPE_BLOB = 1,

} XONLINESTORAGE_CONTENT_TYPE;

// Metadata for one enumerated storage file: owner, content type, wire and
// installed sizes, timestamps, path name, and an attribute blob.
#pragma pack(push, 1)

typedef struct
{
    DWORD dwTitleID;
    DWORD dwTitleVersion;
    ULONGLONG qwOwnerPUID;
    BYTE bCountryID;
    ULONGLONG qwReserved;
    DWORD dwContentType;
    DWORD dwStorageSize;
    DWORD dwInstalledSize;
    FILETIME ftCreated;
    FILETIME ftLastModified;
    WORD wAttributesSize;
    WORD cchPathName;
    LPCWSTR wszPathName;
    PBYTE pbAttributes;

} XONLINESTORAGE_FILE_INFO, *PXONLINESTORAGE_FILE_INFO;

#pragma pack(pop)

// Retrieve the enumerated file records; *pdwTotalResults is the full match
// count, *pdwResultsReturned the number in this page, *prgpStorageFileInfo an
// array of runtime-owned XONLINESTORAGE_FILE_INFO pointers.
XBOXAPI
HRESULT
WINAPI
XOnlineStorageEnumerateGetResults(
    IN XONLINETASK_HANDLE hTask,
    OUT DWORD* pdwTotalResults,
    OUT DWORD* pdwResultsReturned,
    OUT PXONLINESTORAGE_FILE_INFO** prgpStorageFileInfo);

// Progress of an in-flight storage transfer: percent done and/or a
// numerator/denominator byte ratio. All OUT parameters are optional.
XBOXAPI
HRESULT
WINAPI
XOnlineStorageGetProgress(
    IN XONLINETASK_HANDLE hTask,
    OUT OPTIONAL DWORD* pdwPercentDone,
    OUT OPTIONAL ULONGLONG* pqwNumerator,
    OUT OPTIONAL ULONGLONG* pqwDenominator);

// Return the local install path of a previously downloaded storage file.
// *pdwLocationSize is in/out (buffer capacity then written length).
XBOXAPI
HRESULT
WINAPI
XOnlineStorageGetInstallLocation(
    IN DWORD dwFacility,
    IN LPCWSTR wszStoragePath,
    OUT LPSTR szLocation,
    IN OUT DWORD* pdwLocationSize);

// Set the family title ID used to scope subsequent storage operations, letting
// a title share storage with related titles in its family.
XBOXAPI
HRESULT
WINAPI
XOnlineStorageSetFamilyTitleID(
    IN DWORD dwTitleID);


// Launch-data payload for XOnlineTitleUpdateEx: dwContext plus title-defined
// Data carried across the reboot into the update flow.
typedef struct
{
    DWORD dwContext;
    DWORD dwReserved[6];
    BYTE Data[MAX_LAUNCH_DATA_SIZE - 28];

} LD_UPDATE, *PLD_UPDATE;

// Reboot into the security-update flow, passing an LD_UPDATE launch payload.
// The extended form of XOnlineTitleUpdate.
XBOXAPI
HRESULT
WINAPI
XOnlineTitleUpdateEx(
    IN const LD_UPDATE* pldUpdate);

//
// XOnline Offerings
//
// The billing/offering service: query, purchase and cancel downloadable content
// and subscriptions, and enumerate the offerings available or already owned.
// Purchases draw on the payment instrument on file for the account.
//

// Non-USD currency the account may be billed in (a bitmask/format hint used with
// XONLINE_PRICE); absence implies the default currency.
#define XO_CURRENCY_EUR 1
#define XO_CURRENCY_GBP 2
#define XO_CURRENCY_JPY 4
#define XO_CURRENCY_KRW 8


// Tax treatment applied to an offering's price.
typedef enum {
    NO_TAX = 0,
    DEFAULT,
    GST,
    VAT,
    TAX_NOT_APPLICABLE
} XONLINE_TAX_TYPE;


// A localized price: whole and fractional parts, ISO currency code, free flag,
// tax type, and a currency-format hint. Format it for display with
// XOnlineOfferingPriceFormat rather than assembling the string by hand.
typedef struct _XONLINE_PRICE {
    DWORD dwWholePart;
    DWORD dwFractionalPart;
    WCHAR rgwchISOCurrencyCode[3];
    BOOL fOfferingIsFree;
    XONLINE_TAX_TYPE Tax;
    BYTE bCurrencyFormat;
} XONLINE_PRICE, *PXONLINE_PRICE;


// How often a recurring (subscription) offering bills.
typedef enum {
    ONE_TIME_CHARGE = 0,
    MONTHLY,
    QUARTERLY,
    BIANNUALLY,
    ANNUALLY
} XONLINE_OFFERING_FREQUENCY;


// Details of a single offering, returned by XOnlineOfferingDetailsGetResults:
// the title-specific details blob, count already owned, price, and the billing
// schedule for subscriptions.
typedef struct _XONLINEOFFERING_DETAILS {
    PBYTE pbDetailsBuffer; // Pointer to buffer of details blob
    DWORD dwDetailsBuffer; // Length of details blob
    DWORD dwInstances; // Count of currently-owned instances
    XONLINE_PRICE Price; // price structure
    DWORD dwFreeMonthsBeforeCharge; // free months before charge begins
    DWORD dwDuration; // duration of the recurring charge (months)
    XONLINE_OFFERING_FREQUENCY Frequency; // how often charges are made
} XONLINEOFFERING_DETAILS, *PXONLINEOFFERING_DETAILS;


// Purchase an offering for a user, charging the payment instrument on file.
// Async; failures arrive as XONLINE_E_OFFERING_* completion codes.
XBOXAPI
HRESULT
WINAPI
XOnlineOfferingPurchase(
    IN DWORD dwUserIndex,
    IN XOFFERING_ID OfferingId,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Cancel a previously purchased offering/subscription. Async.
XBOXAPI
HRESULT
WINAPI
XOnlineOfferingCancel(
    IN DWORD dwUserIndex,
    IN XOFFERING_ID OfferingId,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Query the price and details of an offering in a given language. Async; the
// buffer is sized via XOnlineOfferingDetailsMaxSize and the parsed result is
// read back with XOnlineOfferingDetailsGetResults.
XBOXAPI
HRESULT
WINAPI
XOnlineOfferingDetails(
    IN DWORD dwUserIndex,
    IN XOFFERING_ID OfferingId,
    IN DWORD dwLanguage,
    IN DWORD dwDescriptionIndex,
    OUT OPTIONAL PBYTE pbBuffer,
    IN DWORD dwBufferSize,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Parse a completed details task into an XONLINEOFFERING_DETAILS.
XBOXAPI
HRESULT
WINAPI
XOnlineOfferingDetailsGetResults(
    IN XONLINETASK_HANDLE hTask,
    OUT PXONLINEOFFERING_DETAILS pDetails);

// Maximum buffer size XOnlineOfferingDetails may need, given the largest
// expected title-specific data size.
XBOXAPI
DWORD
WINAPI
XOnlineOfferingDetailsMaxSize(
    IN DWORD dwTitleSpecificDataMaxSize);

// Format an XONLINE_PRICE into a localized display string. *pdwLength is in/out
// (buffer capacity then length); dwExtendedCharsFilter restricts glyphs a title
// cannot render.
XBOXAPI
HRESULT
WINAPI
XOnlineOfferingPriceFormat(
    IN OUT XONLINE_PRICE* pPrice,
    IN OUT LPWSTR lpFormattedPrice,
    IN OUT DWORD* pdwLength,
    IN DWORD dwExtendedCharsFilter);


//
// Offering enumeration APIs
//

// Country IDs, as stored in the XONLINE_USER_COUNTRY_MASK field of dwUserFlags
// (decode with XOnlineUserCountryId) and used to scope offering availability.
#define XONLINE_COUNTRY_UNITED_ARAB_EMIRATES 1
#define XONLINE_COUNTRY_ALBANIA 2
#define XONLINE_COUNTRY_ARMENIA 3
#define XONLINE_COUNTRY_ARGENTINA 4
#define XONLINE_COUNTRY_AUSTRIA 5
#define XONLINE_COUNTRY_AUSTRALIA 6
#define XONLINE_COUNTRY_AZERBAIJAN 7
#define XONLINE_COUNTRY_BELGIUM 8
#define XONLINE_COUNTRY_BULGARIA 9
#define XONLINE_COUNTRY_BAHRAIN 10
#define XONLINE_COUNTRY_BRUNEI_DARUSSALAM 11
#define XONLINE_COUNTRY_BOLIVIA 12
#define XONLINE_COUNTRY_BRAZIL 13
#define XONLINE_COUNTRY_BELARUS 14
#define XONLINE_COUNTRY_BELIZE 15
#define XONLINE_COUNTRY_CANADA 16
#define XONLINE_COUNTRY_SWITZERLAND 18
#define XONLINE_COUNTRY_CHILE 19
#define XONLINE_COUNTRY_CHINA 20
#define XONLINE_COUNTRY_COLOMBIA 21
#define XONLINE_COUNTRY_COSTA_RICA 22
#define XONLINE_COUNTRY_CZECH_REPUBLIC 23
#define XONLINE_COUNTRY_GERMANY 24
#define XONLINE_COUNTRY_DENMARK 25
#define XONLINE_COUNTRY_DOMINICAN_REPUBLIC 26
#define XONLINE_COUNTRY_ALGERIA 27
#define XONLINE_COUNTRY_ECUADOR 28
#define XONLINE_COUNTRY_ESTONIA 29
#define XONLINE_COUNTRY_EGYPT 30
#define XONLINE_COUNTRY_SPAIN 31
#define XONLINE_COUNTRY_FINLAND 32
#define XONLINE_COUNTRY_FAROE_ISLANDS 33
#define XONLINE_COUNTRY_FRANCE 34
#define XONLINE_COUNTRY_GREAT_BRITAIN 35
#define XONLINE_COUNTRY_GEORGIA 36
#define XONLINE_COUNTRY_GREECE 37
#define XONLINE_COUNTRY_GUATEMALA 38
#define XONLINE_COUNTRY_HONG_KONG 39
#define XONLINE_COUNTRY_HONDURAS 40
#define XONLINE_COUNTRY_CROATIA 41
#define XONLINE_COUNTRY_HUNGARY 42
#define XONLINE_COUNTRY_INDONESIA 43
#define XONLINE_COUNTRY_IRELAND 44
#define XONLINE_COUNTRY_ISRAEL 45
#define XONLINE_COUNTRY_INDIA 46
#define XONLINE_COUNTRY_IRAQ 47
#define XONLINE_COUNTRY_IRAN 48
#define XONLINE_COUNTRY_ICELAND 49
#define XONLINE_COUNTRY_ITALY 50
#define XONLINE_COUNTRY_JAMAICA 51
#define XONLINE_COUNTRY_JORDAN 52
#define XONLINE_COUNTRY_JAPAN 53
#define XONLINE_COUNTRY_KENYA 54
#define XONLINE_COUNTRY_KYRGYZSTAN 55
#define XONLINE_COUNTRY_KOREA 56
#define XONLINE_COUNTRY_KUWAIT 57
#define XONLINE_COUNTRY_KAZAKHSTAN 58
#define XONLINE_COUNTRY_LEBANON 59
#define XONLINE_COUNTRY_LIECHTENSTEIN 60
#define XONLINE_COUNTRY_LITHUANIA 61
#define XONLINE_COUNTRY_LUXEMBOURG 62
#define XONLINE_COUNTRY_LATVIA 63
#define XONLINE_COUNTRY_LIBYA 64
#define XONLINE_COUNTRY_MOROCCO 65
#define XONLINE_COUNTRY_MONACO 66
#define XONLINE_COUNTRY_MACEDONIA 67
#define XONLINE_COUNTRY_MONGOLIA 68
#define XONLINE_COUNTRY_MACAU 69
#define XONLINE_COUNTRY_MALDIVES 70
#define XONLINE_COUNTRY_MEXICO 71
#define XONLINE_COUNTRY_MALAYSIA 72
#define XONLINE_COUNTRY_NICARAGUA 73
#define XONLINE_COUNTRY_NETHERLANDS 74
#define XONLINE_COUNTRY_NORWAY 75
#define XONLINE_COUNTRY_NEW_ZEALAND 76
#define XONLINE_COUNTRY_OMAN 77
#define XONLINE_COUNTRY_PANAMA 78
#define XONLINE_COUNTRY_PERU 79
#define XONLINE_COUNTRY_PHILIPPINES 80
#define XONLINE_COUNTRY_PAKISTAN 81
#define XONLINE_COUNTRY_POLAND 82
#define XONLINE_COUNTRY_PUERTO_RICO 83
#define XONLINE_COUNTRY_PORTUGAL 84
#define XONLINE_COUNTRY_PARAGUAY 85
#define XONLINE_COUNTRY_QATAR 86
#define XONLINE_COUNTRY_ROMANIA 87
#define XONLINE_COUNTRY_RUSSIAN_FEDERATION 88
#define XONLINE_COUNTRY_SAUDI_ARABIA 89
#define XONLINE_COUNTRY_SWEDEN 90
#define XONLINE_COUNTRY_SINGAPORE 91
#define XONLINE_COUNTRY_SLOVENIA 92
#define XONLINE_COUNTRY_SLOVAK_REPUBLIC 93
#define XONLINE_COUNTRY_EL_SALVADOR 95
#define XONLINE_COUNTRY_SYRIA 96
#define XONLINE_COUNTRY_THAILAND 97
#define XONLINE_COUNTRY_TUNISIA 98
#define XONLINE_COUNTRY_TURKEY 99
#define XONLINE_COUNTRY_TRINIDAD_AND_TOBAGO 100
#define XONLINE_COUNTRY_TAIWAN 101
#define XONLINE_COUNTRY_UKRAINE 102
#define XONLINE_COUNTRY_UNITED_STATES 103
#define XONLINE_COUNTRY_URUGUAY 104
#define XONLINE_COUNTRY_UZBEKISTAN 105
#define XONLINE_COUNTRY_VENEZUELA 106
#define XONLINE_COUNTRY_VIET_NAM 107
#define XONLINE_COUNTRY_YEMEN 108
#define XONLINE_COUNTRY_SOUTH_AFRICA 109
#define XONLINE_COUNTRY_ZIMBABWE 110


// Offering type filter bits for enumeration: subscriptions and/or one-off
// content. _ALL matches every type or every publisher bit.
#define XONLINE_OFFERING_SUBSCRIPTION 0x1
#define XONLINE_OFFERING_CONTENT 0x2


#define XONLINE_OFFERING_BITFILTER_ALL 0xffffffff
#define XONLINE_OFFERING_TYPE_ALL 0xffffffff


// Bit in XONLINEOFFERING_INFO.fOfferingFlags; test it with XOnlineOfferingIsFree.
#define XONLINE_OFFERING_IS_NOT_FREE 0x1


#define XOnlineOfferingIsFree(x) (((x) & XONLINE_OFFERING_IS_NOT_FREE) == 0)


// Filter/paging parameters for XOnlineOfferingEnumerate: which offering types
// and publisher-defined bits to match, and the result window to return.
#pragma pack(push, 1)
typedef struct _XONLINEOFFERING_ENUM_PARAMS {
    DWORD dwOfferingType; // Filter on offering type
    DWORD dwBitFilter; // Bitfield for filtering offerings
    DWORD dwDescriptionIndex; // Publisher-specific index
    WORD wStartingIndex; // Starting index to enumerate
    WORD wMaxResults; // Desired max number of results
} XONLINEOFFERING_ENUM_PARAMS, *PXONLINEOFFERING_ENUM_PARAMS;
#pragma pack(pop)


// One offering returned by enumeration: its ID, type, sizes, activation date,
// rating, flags, and a title-specific data blob.
#pragma pack(push, 1)
typedef struct _XONLINEOFFERING_INFO {
    XOFFERING_ID OfferingId; // Offering ID
    DWORD dwOfferingType; // Offering type
    DWORD dwBitFlags; // Package-specific flags
    DWORD dwPackageSize; // Package wire size (bytes)
    DWORD dwInstallSize; // Installed size (blocks)
    FILETIME ftActivationDate; // Activation date of package
    DWORD dwRating; // Package rating
    WORD fOfferingFlags; // Per-offering flags
    DWORD dwTitleSpecificData; // Size of data blob (bytes)
    PBYTE pbTitleSpecificData; // Pointer to data blob
} XONLINEOFFERING_INFO, *PXONLINEOFFERING_INFO;
#pragma pack(pop)

#define XONLINEOFFERING_ENUM_MAX_TITLE_DATA_SIZE (8000)


// Check whether any new content matching dwBitFilter is available for the user.
// Async; completes with XONLINE_S_OFFERING_NEW_CONTENT or _NO_NEW_CONTENT.
XBOXAPI
HRESULT
WINAPI
XOnlineOfferingIsNewContentAvailable(
    IN DWORD dwBitFilter,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);


// Enumerate offerings matching pEnumParams into a caller buffer (size it with
// XOnlineOfferingEnumerateMaxSize). Async; parse with the GetResults call.
XBOXAPI
HRESULT
WINAPI
XOnlineOfferingEnumerate(
    IN DWORD dwUserIndex,
    IN const XONLINEOFFERING_ENUM_PARAMS* pEnumParams,
    OUT OPTIONAL PBYTE pbBuffer,
    IN DWORD dwBufferSize,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);


// Retrieve the enumerated offerings as an array of XONLINEOFFERING_INFO pointers;
// *pfMoreResults indicates another page can be fetched.
XBOXAPI
HRESULT
WINAPI
XOnlineOfferingEnumerateGetResults(
    IN XONLINETASK_HANDLE hTask,
    OUT PXONLINEOFFERING_INFO** prgpOfferingInfo,
    OUT DWORD* pdwReturnedResults,
    OUT BOOL* pfMoreResults);


// Buffer size XOnlineOfferingEnumerate needs for the given params and expected
// per-offering title data.
XBOXAPI
DWORD
WINAPI
XOnlineOfferingEnumerateMaxSize(
    IN const XONLINEOFFERING_ENUM_PARAMS* pEnumParams,
    IN OPTIONAL DWORD dwTitleSpecificDataMaxSize);


// Download and install owned content for an offering. Async; track it with
// XOnlineContentInstallGetProgress and query its footprint with GetSize.
XBOXAPI
HRESULT
WINAPI
XOnlineContentInstall(
    IN XOFFERING_ID OfferingId,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);


// Progress of a content install (percent and/or byte ratio). OUT params optional.
XBOXAPI
HRESULT
WINAPI
XOnlineContentInstallGetProgress(
    IN XONLINETASK_HANDLE hTask,
    OUT OPTIONAL DWORD* pdwPercentDone,
    OUT OPTIONAL ULONGLONG* pqwNumerator,
    OUT OPTIONAL ULONGLONG* pqwDenominator);

// Report an install's total installed size and additional blocks still needed,
// so a title can check for free space before committing.
XBOXAPI
HRESULT
WINAPI
XOnlineContentInstallGetSize(
    IN XONLINETASK_HANDLE hTask,
    OUT DWORD* pdwTotalInstalledSizeInBlocks,
    OUT DWORD* pdwAdditionalBlocksRequired);

// Provide the title's secret key used to decrypt installed content.
XBOXAPI
HRESULT
WINAPI
XOnlineContentSetSecurityKey(
    IN const BYTE* pbSecretKey);


//
// Messaging
//
// Inter-user messages (friend requests, game invites, team recruits, custom
// title messages, Live system messages). A message is a typed envelope
// (XONLINE_MSG_TYPE_*) carrying a set of tagged properties (text, voice,
// session ID, and so on). Incoming messages surface first as lightweight
// XONLINE_MSG_SUMMARY records via XOnlineMessageEnumerate; the full property
// set and any storage attachments are fetched on demand with
// XOnlineMessageDetails. Outgoing messages are built with
// XOnlineMessageCreate / XOnlineMessageSetProperty and sent with
// XOnlineMessageSend.
//

// Inbox capacity, per-send recipient cap, and the maximum inline details size
// (larger payloads become downloadable attachments).
#define XONLINE_MAX_NUM_MESSAGES 125
#define XONLINE_MAX_MESSAGE_RECIPIENTS 100
#define XONLINE_MAX_MESSAGE_DETAILS 4096

DECLARE_HANDLE(XONLINE_MSG_HANDLE);
typedef XONLINE_MSG_HANDLE* PXONLINE_MSG_HANDLE;

//
// Message Flags
//
#define XONLINE_MSG_FLAG_REQUIRED 0x00000001 // The user is required to read this message
#define XONLINE_MSG_FLAG_RECOMMENDED 0x00000002 // The user has a system recommended message
#define XONLINE_MSG_FLAG_HAS_VOICE 0x00000004 // This message contains a voice attachment
#define XONLINE_MSG_FLAG_HAS_TEXT 0x00000008 // This message contains a text body
#define XONLINE_MSG_FLAG_READ 0x00000010 // This message has been read
#define XONLINE_MSG_FLAG_NON_EXPORTABLE 0x00000020 // This message should only be displayed on Xbox consoles, not the web
#define XONLINE_MSG_FLAG_TEAM_CONTEXT 0x00000040 // This message's sender context refers to a team ID
#define XONLINE_MSG_FLAG_COMP_CONTEXT 0x00000080 // This message's sender context refers to a competition event or entity ID
#define XONLINE_MSG_FLAG_ALTERNATE_TITLE 0x00000100 // This message is from an alternate Title of the currently logged on title (only available on Xbox consoles)
#define XONLINE_MSG_FLAG_MARKETING 0x00000200 // Used on title-sent messages to indicate the message is marketing-related and opt-in settings should be checked
#define XONLINE_MSG_FLAGS_TITLE_RESERVED 0xFF000000 // Flags reserved for title custom messages

//
// Message Property Types
//
#define XONLINE_MSG_PROP_TYPE_NULL ((BYTE)1) // The property contains no data
#define XONLINE_MSG_PROP_TYPE_I1 ((BYTE)2) // The property value points to 8-bits of data
#define XONLINE_MSG_PROP_TYPE_I2 ((BYTE)3) // The property value points to 16-bits of data
#define XONLINE_MSG_PROP_TYPE_I4 ((BYTE)4) // The property value points to 32-bits of data
#define XONLINE_MSG_PROP_TYPE_I8 ((BYTE)5) // The property value points to 64-bits of data
#define XONLINE_MSG_PROP_TYPE_STRING ((BYTE)6) // The property value points to a NULL-terminated wide character string
#define XONLINE_MSG_PROP_TYPE_FILETIME ((BYTE)7) // The property value points to a time value
#define XONLINE_MSG_PROP_TYPE_BINARY ((BYTE)8) // The property value points to a binary blob that fits in the message details
#define XONLINE_MSG_PROP_TYPE_ATTACHMENT ((BYTE)9) // The property value points to a binary blob (or directory path, depending on attachment flags) that will be uploaded to storage during XOnlineMessageSend
#define XONLINE_MSG_PROP_TYPE_BOOL ((BYTE)10) // The property value points to a boolean value (1=TRUE, 0=FALSE)
#define XONLINE_MSG_PROP_TYPE_STRING_ID ((BYTE)11) // The property value contains a string ID whose text can be requested from the string service

//
// Attachment Flags
//
#define XONLINE_MSG_ATTACHMENT_FLAG_NON_EXPORTABLE 0x00000001 // This attachment should not be visible when the message is displayed on the web
#define XONLINE_MSG_ATTACHMENT_FLAG_DIRECTORY 0x00000002 // The property value points to an ANSI path string to a local directory that will be uploaded to storage, instead of a binary blob


//
// Message Property Tags
// Property tags occupy a word where the upper byte indicates the property's data type, and
// the lower byte indicates a unique identifier for the property.  Property identifiers only
// need to be unique within a given message type.
//
#define XONLINE_MSG_PROP_TAG(type, id) ((BYTE)(type) << 8 | (BYTE)(id))
#define XOnlineMessageGetPropId(tag) (BYTE)((WORD)(tag) & 0xFF)
#define XOnlineMessageGetPropType(tag) (BYTE)((WORD)(tag) >> 8)

//
// Message Property IDs
// Property IDs should adhere to the following ranges:
//
// Range           Purpose
// 0x00-0x7F       Reserved for use by titles
// 0x80-0xBF       Reserved for use by Microsoft for non-global properties
// 0xC0-0xFF       Reserved for use by Microsoft for properties that span all message types
#define XONLINE_MSG_PROP_ID_BUILTIN 0x80 // Used for properties defined by Microsoft
#define XONLINE_MSG_PROP_ID_GLOBAL 0x40 // Used for properties that can span all message types


//
// Message Types
//
// A message type establishes the purpose of the message and the properties that
// are required for a message of that type.
//
#define XONLINE_MSG_TYPE_TITLE_CUSTOM ((BYTE)1) // context: title defined;    required props: title defined
#define XONLINE_MSG_TYPE_FRIEND_REQUEST ((BYTE)2) // context: 0;                required props: none
#define XONLINE_MSG_TYPE_GAME_INVITE ((BYTE)3) // context: see msg flags;    required props: XONLINE_MSG_PROP_SESSION_ID
#define XONLINE_MSG_TYPE_TEAM_RECRUIT ((BYTE)4) // context: inviting team ID; required props: none
#define XONLINE_MSG_TYPE_COMP_REMINDER ((BYTE)5) // context: comp event ID;    required props: XONLINE_MSG_PROP_COMP_NAME, XONLINE_MSG_PROP_COMP_EVENT_START
#define XONLINE_MSG_TYPE_COMP_REQUEST ((BYTE)6) // context: comp entity ID;   required props: XONLINE_MSG_PROP_COMP_NAME, XONLINE_MSG_PROP_COMP_START, XONLINE_MSG_PROP_COMP_REG_CLOSE
#define XONLINE_MSG_TYPE_LIVE_MESSAGE ((BYTE)7) // context: 0                 required props: XONLINE_MSG_PROP_SYSTEM_TEXT

//
// Global property tags, allowed in any message type
//

// All message types can contain a voice attachment specified in the following properties.  All three
// of the following properties should be specified together.  The XONLINE_MSG_FLAG_HAS_VOICE message
// flag can be used to test for the presence of these properties before the message details have been
// retrieved.
#define XONLINE_MSG_PROP_VOICE_DATA XONLINE_MSG_PROP_TAG(XONLINE_MSG_PROP_TYPE_ATTACHMENT, XONLINE_MSG_PROP_ID_GLOBAL | XONLINE_MSG_PROP_ID_BUILTIN | 1)
#define XONLINE_MSG_PROP_VOICE_DATA_CODEC XONLINE_MSG_PROP_TAG(XONLINE_MSG_PROP_TYPE_I2, XONLINE_MSG_PROP_ID_GLOBAL | XONLINE_MSG_PROP_ID_BUILTIN | 2)
#define XONLINE_MSG_PROP_VOICE_DATA_DURATION XONLINE_MSG_PROP_TAG(XONLINE_MSG_PROP_TYPE_I4, XONLINE_MSG_PROP_ID_GLOBAL | XONLINE_MSG_PROP_ID_BUILTIN | 3)

// All message types can contain a text summary specified in the following properties.  Both
// of the following properties should be specified together.  The XONLINE_MSG_FLAG_HAS_TEXT message
// flag can be used to test for the presence of these properties before the message details have been
// retrieved.  The text should be 256 characters or less and may be used by Live anywhere that a text
// summary of the message is useful, such as in a Messenger Alert, on the web, or in the Dashboard.
#define XONLINE_MSG_PROP_TEXT XONLINE_MSG_PROP_TAG(XONLINE_MSG_PROP_TYPE_STRING, XONLINE_MSG_PROP_ID_GLOBAL | XONLINE_MSG_PROP_ID_BUILTIN | 4)
#define XONLINE_MSG_PROP_TEXT_LANGUAGE XONLINE_MSG_PROP_TAG(XONLINE_MSG_PROP_TYPE_I4, XONLINE_MSG_PROP_ID_GLOBAL | XONLINE_MSG_PROP_ID_BUILTIN | 5)

//
// Invite message properties
//
#define XONLINE_MSG_PROP_SESSION_ID XONLINE_MSG_PROP_TAG(XONLINE_MSG_PROP_TYPE_I8, XONLINE_MSG_PROP_ID_BUILTIN | 1)

//
// Live system message properties
//
#define XONLINE_MSG_PROP_SYSTEM_TEXT XONLINE_MSG_PROP_TAG(XONLINE_MSG_PROP_TYPE_STRING_ID, XONLINE_MSG_PROP_ID_BUILTIN | 1)

//
// Team Recruit message properties
//
#define XONLINE_MSG_PROP_TEAM_NAME XONLINE_MSG_PROP_TAG(XONLINE_MSG_PROP_TYPE_STRING, XONLINE_MSG_PROP_ID_BUILTIN | 1)

//
// Competitions message properties
//
#define XONLINE_MSG_PROP_COMP_DATASET XONLINE_MSG_PROP_TAG(XONLINE_MSG_PROP_TYPE_I4, XONLINE_MSG_PROP_ID_BUILTIN | 0x1)
#define XONLINE_MSG_PROP_COMP_NAME XONLINE_MSG_PROP_TAG(XONLINE_MSG_PROP_TYPE_STRING, XONLINE_MSG_PROP_ID_BUILTIN | 0x2)
#define XONLINE_MSG_PROP_COMP_START XONLINE_MSG_PROP_TAG(XONLINE_MSG_PROP_TYPE_FILETIME, XONLINE_MSG_PROP_ID_BUILTIN | 0x3)
#define XONLINE_MSG_PROP_COMP_ROUND XONLINE_MSG_PROP_TAG(XONLINE_MSG_PROP_TYPE_I2, XONLINE_MSG_PROP_ID_BUILTIN | 0x4)
#define XONLINE_MSG_PROP_COMP_OPPONENT XONLINE_MSG_PROP_TAG(XONLINE_MSG_PROP_TYPE_STRING_ID, XONLINE_MSG_PROP_ID_BUILTIN | 0x5)
#define XONLINE_MSG_PROP_COMP_ADMIN XONLINE_MSG_PROP_TAG(XONLINE_MSG_PROP_TYPE_I8, XONLINE_MSG_PROP_ID_BUILTIN | 0x6)
#define XONLINE_MSG_PROP_COMP_REG_CLOSE XONLINE_MSG_PROP_TAG(XONLINE_MSG_PROP_TYPE_FILETIME, XONLINE_MSG_PROP_ID_BUILTIN | 0x7)
#define XONLINE_MSG_PROP_COMP_PRIVATE_SLOTS XONLINE_MSG_PROP_TAG(XONLINE_MSG_PROP_TYPE_I2, XONLINE_MSG_PROP_ID_BUILTIN | 0x8)
#define XONLINE_MSG_PROP_COMP_PUBLIC_SLOTS XONLINE_MSG_PROP_TAG(XONLINE_MSG_PROP_TYPE_I2, XONLINE_MSG_PROP_ID_BUILTIN | 0x9)
#define XONLINE_MSG_PROP_COMP_UNITS XONLINE_MSG_PROP_TAG(XONLINE_MSG_PROP_TYPE_I2, XONLINE_MSG_PROP_ID_BUILTIN | 0xA)
#define XONLINE_MSG_PROP_COMP_INTERVAL XONLINE_MSG_PROP_TAG(XONLINE_MSG_PROP_TYPE_I2, XONLINE_MSG_PROP_ID_BUILTIN | 0xB)
#define XONLINE_MSG_PROP_COMP_DAYMASK XONLINE_MSG_PROP_TAG(XONLINE_MSG_PROP_TYPE_I2, XONLINE_MSG_PROP_ID_BUILTIN | 0xC)
#define XONLINE_MSG_PROP_COMP_DESCRIPTION XONLINE_MSG_PROP_TAG(XONLINE_MSG_PROP_TYPE_STRING, XONLINE_MSG_PROP_ID_BUILTIN | 0xD)
#define XONLINE_MSG_PROP_COMP_URL XONLINE_MSG_PROP_TAG(XONLINE_MSG_PROP_TYPE_STRING, XONLINE_MSG_PROP_ID_BUILTIN | 0xE)
#define XONLINE_MSG_PROP_COMP_EVENT_ID XONLINE_MSG_PROP_TAG(XONLINE_MSG_PROP_TYPE_I8, XONLINE_MSG_PROP_ID_BUILTIN | 0x10)
#define XONLINE_MSG_PROP_COMP_EVENT_START XONLINE_MSG_PROP_TAG(XONLINE_MSG_PROP_TYPE_FILETIME, XONLINE_MSG_PROP_ID_BUILTIN | 0x11)

//
// Voice Codec types (values for XONLINE_MSG_PROP_VOICE_DATA_CODEC)
//
#define XONLINE_PROP_VOICE_DATA_CODEC_WMAVOICE_V90 1


// Lightweight header for one message (sender, type, context, timestamps, flags,
// details size). Returned by XOnlineMessageEnumerate/Summary without fetching
// the message body.
#pragma pack(push, 8)
typedef struct _XONLINE_MSG_SUMMARY {
    XUID xuidSender; // User ID of sender
    BYTE bMsgType; // Type of the message
    ULONGLONG qwMessageContext; // Message specific context value
    FILETIME ftSentTime; // Time at which message was sent, in Coordinated Universal Time (UTC) format
    DWORD dwMessageID; // ID of message
    DWORD dwMessageFlags; // Flags describing message
    DWORD dwSenderTitleID; // ID of title in which message was sent
    WORD wExpireMinutes; // An offset in minutes from the sent time
    WORD cbDetails; // Size of details blob, excluding downloadable content
    char szSenderName[XONLINE_GAMERTAG_SIZE]; // Gamer tag of sender
} XONLINE_MSG_SUMMARY, *PXONLINE_MSG_SUMMARY;
#pragma pack(pop)

// Per-recipient outcome of a send: whom, the HRESULT, and (on success) the
// assigned message ID. One entry per recipient; also the input to
// XOnlineMessageRevoke.
typedef struct _XONLINE_MSG_SEND_RESULT {
    XUID xuidRecipient; // User ID of recipient
    HRESULT hr; // Result code of send to recipient
    DWORD dwMessageID; // Message ID of send to recipient, if successful
} XONLINE_MSG_SEND_RESULT, *PXONLINE_MSG_SEND_RESULT;


// Enumerate the summaries of the user's inbox messages into pMsgSummaries
// (capacity/count via *pdwNumMsgSummaries). Synchronous, from the local cache.
XBOXAPI
HRESULT
WINAPI
XOnlineMessageEnumerate(
    IN DWORD dwUserIndex,
    OUT XONLINE_MSG_SUMMARY* pMsgSummaries,
    OUT DWORD* pdwNumMsgSummaries);


// Fetch the cached summary for one message by ID.
XBOXAPI
HRESULT
WINAPI
XOnlineMessageSummary(
    IN DWORD dwUserIndex,
    IN DWORD dwMessageID,
    OUT XONLINE_MSG_SUMMARY* pMsgSummary);


// Download a message's full property set (and optionally mark flags read).
// Async; the same task handle is then used with the GetResults* property
// readers and with XOnlineMessageDownloadAttachment* for large attachments.
XBOXAPI
HRESULT
WINAPI
XOnlineMessageDetails(
    IN DWORD dwUserIndex,
    IN DWORD dwMessageID,
    IN DWORD dwMessageFlagsToSet,
    IN DWORD dwMessageFlagsToClear,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);


// From a completed details task, read the summary, the property count, and the
// total size of downloadable attachments.
XBOXAPI
HRESULT
WINAPI
XOnlineMessageDetailsGetResultsSummary(
    IN XONLINETASK_HANDLE hTask,
    OUT OPTIONAL XONLINE_MSG_SUMMARY* pMsgSummary,
    OUT OPTIONAL DWORD* pdwNumProperties,
    OUT OPTIONAL ULONGLONG* pqwAttachmentsSize);


// Read one property value by tag from a completed details task. If the property
// is an attachment too large to inline, *pdwAttachmentFlags is set and the data
// must be fetched with XOnlineMessageDownloadAttachment*.
XBOXAPI
HRESULT
WINAPI
XOnlineMessageDetailsGetResultsProperty(
    IN XONLINETASK_HANDLE hTask,
    IN WORD wPropTag,
    IN DWORD dwPropValueBufferSize,
    IN OUT OPTIONAL PVOID pPropValue,
    OUT OPTIONAL DWORD* pdwPropValueSize,
    OUT OPTIONAL DWORD* pdwAttachmentFlags);


// Download an attachment property into memory. Started from the details task;
// results via the matching GetResults call. Async.
XBOXAPI
HRESULT
WINAPI
XOnlineMessageDownloadAttachmentToMemory(
    IN XONLINETASK_HANDLE hDetailsTask,
    IN WORD wPropTag,
    IN OUT PBYTE pbBuffer,
    IN DWORD dwBufferSize,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phDownloadTask);


// Results of an attachment-to-memory download: data pointer/size and total size.
XBOXAPI
HRESULT
WINAPI
XOnlineMessageDownloadAttachmentToMemoryGetResults(
    IN XONLINETASK_HANDLE hDownloadTask,
    OUT OPTIONAL PBYTE* ppbReceivedData,
    OUT OPTIONAL DWORD* pdwReceivedDataSize,
    OUT OPTIONAL DWORD* pdwTotalDataSize);


// Download an attachment property directly to a local directory. Async.
XBOXAPI
HRESULT
WINAPI
XOnlineMessageDownloadAttachmentToDirectory(
    IN XONLINETASK_HANDLE hDetailsTask,
    IN WORD wPropTag,
    IN LPCSTR lpLocalPath,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phDownloadTask);


// Progress of an attachment download (percent and/or byte ratio). OUT optional.
XBOXAPI
HRESULT
WINAPI
XOnlineMessageDownloadAttachmentGetProgress(
    IN XONLINETASK_HANDLE hDownloadTask,
    OUT OPTIONAL DWORD* pdwPercentDone,
    OUT OPTIONAL ULONGLONG* pqwNumerator,
    OUT OPTIONAL ULONGLONG* pqwDenominator);


// Set and/or clear message flags (e.g. mark read) on the server. Async.
XBOXAPI
HRESULT
WINAPI
XOnlineMessageSetFlags(
    IN DWORD dwUserIndex,
    IN DWORD dwMessageID,
    IN DWORD dwMessageFlagsToSet,
    IN DWORD dwMessageFlagsToClear,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);


// Delete a message locally and on the server, optionally blocking further
// messages from its sender. Async is not used; returns directly.
XBOXAPI
HRESULT
WINAPI
XOnlineMessageDelete(
    IN DWORD dwUserIndex,
    IN DWORD dwMessageID,
    IN BOOL fBlockSender);


// Build a new outgoing message of the given type, pre-sizing it for
// wNumProperties properties totaling wExpectedValuesSize bytes. Populate it with
// XOnlineMessageSetProperty, send with XOnlineMessageSend, and free with
// XOnlineMessageDestroy.
XBOXAPI
HRESULT
WINAPI
XOnlineMessageCreate(
    IN BYTE bMsgType,
    IN WORD wNumProperties,
    IN WORD wExpectedValuesSize,
    IN ULONGLONG qwMessageContext,
    IN DWORD dwMessageFlags,
    IN WORD wExpireMinutes,
    OUT XONLINE_MSG_HANDLE* phMsg);


// Free a message built with XOnlineMessageCreate.
XBOXAPI
HRESULT
WINAPI
XOnlineMessageDestroy(
    IN XONLINE_MSG_HANDLE hMsg);


// Add or replace one property (tagged by wPropTag) on an outgoing message;
// dwAttachmentFlags apply when the property is an attachment.
XBOXAPI
HRESULT
WINAPI
XOnlineMessageSetProperty(
    IN XONLINE_MSG_HANDLE hMsg,
    IN WORD wPropTag,
    IN DWORD dwPropValueSize,
    IN const VOID* pPropValue,
    IN DWORD dwAttachmentFlags);


// Send a built message to up to XONLINE_MAX_MESSAGE_RECIPIENTS users. Async;
// per-recipient outcomes come from XOnlineMessageSendGetResults.
XBOXAPI
HRESULT
WINAPI
XOnlineMessageSend(
    IN DWORD dwUserIndexSender,
    IN XONLINE_MSG_HANDLE hMsg,
    IN DWORD dwRecipientCount,
    IN const XUID* pxuidRecipients,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);


// Progress of a send that is uploading attachments (percent and/or byte ratio).
XBOXAPI
HRESULT
WINAPI
XOnlineMessageSendGetProgress(
    IN XONLINETASK_HANDLE hTask,
    OUT OPTIONAL DWORD* pdwPercentDone,
    OUT OPTIONAL ULONGLONG* pqwNumerator,
    OUT OPTIONAL ULONGLONG* pqwDenominator);


// Retrieve one XONLINE_MSG_SEND_RESULT per recipient from a completed send.
XBOXAPI
HRESULT
WINAPI
XOnlineMessageSendGetResults(
    IN XONLINETASK_HANDLE hTask,
    OUT XONLINE_MSG_SEND_RESULT* pMsgSendResults);


// Recall previously sent messages, identified by the send-result records.
// Async. Fails with XONLINE_E_MESSAGE_NO_VALID_SENDS_TO_REVOKE if none qualify.
XBOXAPI
HRESULT
WINAPI
XOnlineMessageRevoke(
    IN DWORD dwUserIndex,
    IN DWORD dwNumMsgSendResults,
    IN const XONLINE_MSG_SEND_RESULT* pMsgSendResults,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Enable or disable automatic background refresh of the message summary list.
// Returns the previous setting.
XBOXAPI
BOOL
    WINAPI
    XOnlineMessageSetSummaryRefresh(
        IN BOOL fEnable);

// Allow this title to receive messages sent by the listed family title IDs.
XBOXAPI
HRESULT
WINAPI
XOnlineMessageEnableReceivingFamilyTitleIDs(
    IN DWORD dwNumTitleIDs,
    IN const DWORD* pdwTitleIDs);

// Set the family title ID stamped on messages this title sends.
XBOXAPI
HRESULT
WINAPI
XOnlineMessageSetSendingFamilyTitleID(
    IN DWORD dwTitleID);


//
// XOnline Matchmaking
//
// Advertise and find multiplayer game sessions. A host creates a session
// described by a set of typed attributes (XONLINE_ATTRIBUTE) and public/private
// slot counts; other consoles run a stored search procedure to find matching
// sessions and receive XONLINE_MATCH_SEARCHRESULT records with the host's XNADDR
// and key-exchange key needed to connect. The same XONLINE_ATTRIBUTE model is
// reused by the query, competition, and arbitration services.
//

// A single matchmaking attribute: an ID (whose data-type field selects the
// active union member) and its integer, string, or blob value. fChanged marks
// attributes to update in XOnlineMatchSessionUpdate.
typedef struct _XONLINE_ATTRIBUTE {
    DWORD dwAttributeID;
    BOOL fChanged;
    union {
        struct {
            ULONGLONG qwValue;
        } integer;
        struct {
            LPWSTR lpValue;
        } string;
        struct {
            PVOID pvValue;
            DWORD dwLength;
        } blob;
    } info;
} XONLINE_ATTRIBUTE, *PXONLINE_ATTRIBUTE;

// Describes one result column to return from a search: its data type and, for
// string/blob columns, the reserved length. An array of these shapes the
// result buffer.
typedef struct _XONLINE_ATTRIBUTE_SPEC {
    DWORD dwType;
    DWORD dwLength;
} XONLINE_ATTRIBUTE_SPEC, *PXONLINE_ATTRIBUTE_SPEC;

// Attribute size limits and the bitfields packed into an attribute ID: the
// data-type nibble (integer/string/blob/null), the title-specific scope, and
// the low 16-bit identifier.
#define X_MAX_STRING_ATTRIBUTE_LEN 400
#define X_MAX_BLOB_ATTRIBUTE_LEN 800

#define X_ATTRIBUTE_SCOPE_TITLE_SPECIFIC 0x00000000

#define X_ATTRIBUTE_DATATYPE_MASK 0x00F00000
#define X_ATTRIBUTE_DATATYPE_INTEGER 0x00000000
#define X_ATTRIBUTE_DATATYPE_STRING 0x00100000
#define X_ATTRIBUTE_DATATYPE_BLOB 0x00200000
#define X_ATTRIBUTE_DATATYPE_NULL 0x00F00000

#define X_ATTRIBUTE_ID_MASK 0x0000FFFF

// One found session: its ID, the host's address and key-exchange key (to
// establish a secure XNet connection), current public/private slot occupancy,
// and attribute count. Parse a result buffer into these with
// XOnlineMatchSearchParse.
#pragma pack(push, 1)

typedef struct _XONLINE_MATCH_SEARCHRESULT {
    DWORD dwReserved;
    XNKID SessionID;
    XNADDR HostAddress;
    XNKEY KeyExchangeKey;
    DWORD dwPublicOpen;
    DWORD dwPrivateOpen;
    DWORD dwPublicFilled;
    DWORD dwPrivateFilled;
    DWORD dwNumAttributes;
} XONLINE_MATCH_SEARCHRESULT, *PXONLINE_MATCH_SEARCHRESULT;

#pragma pack(pop)

// Create and advertise a new matchmaking session with the given slot counts and
// attributes. Async; the assigned session ID and key come from
// XOnlineMatchSessionGetInfo.
XBOXAPI
HRESULT
WINAPI
XOnlineMatchSessionCreate(
    IN DWORD dwPublicFilled,
    IN DWORD dwPublicOpen,
    IN DWORD dwPrivateFilled,
    IN DWORD dwPrivateOpen,
    IN DWORD dwNumAttributes,
    IN OUT PXONLINE_ATTRIBUTE pAttributes,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Update a session's slot counts and changed attributes (mark them with
// fChanged) as the game fills or its state changes. Async.
XBOXAPI
HRESULT
WINAPI
XOnlineMatchSessionUpdate(
    IN XNKID SessionID,
    IN DWORD dwPublicFilled,
    IN DWORD dwPublicOpen,
    IN DWORD dwPrivateFilled,
    IN DWORD dwPrivateOpen,
    IN DWORD dwNumAttributes,
    IN OUT PXONLINE_ATTRIBUTE pAttributes,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Retrieve the server-assigned session ID and key-exchange key from a completed
// create/find task.
XBOXAPI
HRESULT
WINAPI
XOnlineMatchSessionGetInfo(
    IN XONLINETASK_HANDLE hTask,
    OUT XNKID* pSessionID,
    OUT XNKEY* pKeyExchangeKey);

// Remove an advertised session (host tear-down). Async.
XBOXAPI
HRESULT
WINAPI
XOnlineMatchSessionDelete(
    IN XNKID SessionID,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Look up a specific session by ID (e.g. to join a friend's game). Async;
// details via XOnlineMatchSessionGetInfo.
XBOXAPI
HRESULT
WINAPI
XOnlineMatchSessionFindFromID(
    IN XNKID SessionID,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Run a stored search procedure with query attributes to find sessions.
// dwResultsLen sizes the result buffer (see XOnlineMatchSearchResultsLen).
// Async; results via XOnlineMatchSearchGetResults.
XBOXAPI
HRESULT
WINAPI
XOnlineMatchSearch(
    IN DWORD dwProcedureIndex,
    IN DWORD dwNumResults,
    IN DWORD dwNumAttributes,
    IN const XONLINE_ATTRIBUTE* pAttributes,
    IN DWORD dwResultsLen,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Retrieve the array of XONLINE_MATCH_SEARCHRESULT pointers from a completed
// search.
XBOXAPI
HRESULT
WINAPI
XOnlineMatchSearchGetResults(
    IN XONLINETASK_HANDLE hTask,
    OUT PXONLINE_MATCH_SEARCHRESULT** prgpSearchResults,
    OUT DWORD* pdwReturnedResults);

// Expand one search result's attribute blob into a caller struct described by
// the attribute spec array.
XBOXAPI
HRESULT
WINAPI
XOnlineMatchSearchParse(
    IN const XONLINE_MATCH_SEARCHRESULT* pSearchResult,
    IN DWORD dwNumSessionAttributes,
    IN const XONLINE_ATTRIBUTE_SPEC* pSessionAttributeSpec,
    OUT PVOID pQuerySession);

// Compute the result-buffer length XOnlineMatchSearch needs for the requested
// result count and per-session attribute spec.
XBOXAPI
DWORD
WINAPI
XOnlineMatchSearchResultsLen(
    IN DWORD dwNumResults,
    IN DWORD dwNumSessionAttributes,
    IN const XONLINE_ATTRIBUTE_SPEC* pSessionAttributeSpec);


//
// Title Name
//

// Localized title name buffer: up to MAX_TITLENAME_LEN wide characters.
#define MAX_TITLENAME_LEN 40
#define MAX_TITLENAME_SIZE (MAX_TITLENAME_LEN * sizeof(WCHAR))


// TRUE if dwTitleID is this same title (any version).
BOOL
    WINAPI
    XOnlineTitleIdIsSameTitle(
        IN DWORD dwTitleID);

// TRUE if dwTitleID belongs to this title's publisher.
BOOL
    WINAPI
    XOnlineTitleIdIsSamePublisher(
        IN DWORD dwTitleID);

//
// Notification
//
// Fast, poll-based flags for pending social events, driven by the presence
// connection. A title polls XOnlineGetNotification(Ex) each frame to know when
// to refresh friends, show an invite, etc., and publishes its own joinable
// state with XOnlineNotificationSetState.
//

// Kinds of pending notification queried by XOnlineGetNotification.
typedef enum {
    XONLINE_NOTIFICATION_FRIEND_REQUEST = 0,
    XONLINE_NOTIFICATION_GAME_INVITE = 1,
    XONLINE_NOTIFICATION_NEW_GAME_INVITE = 2,
    XONLINE_NOTIFICATION_GAME_INVITE_ANSWER = 3,
    XONLINE_NOTIFICATION_NUM = 4
} XONLINE_NOTIFICATION_TYPE;

// Publish the user's current session/joinable state and a title-defined state
// data blob to friends via the presence channel.
XBOXAPI
HRESULT
WINAPI
XOnlineNotificationSetState(
    IN DWORD dwUserIndex,
    IN DWORD dwStateFlags,
    IN XNKID sessionID,
    IN DWORD dwStateData,
    IN const BYTE* pbStateData);

// TRUE if a notification of the given type is pending for the user. Poll this
// cheaply to decide when to run the heavier friends/messaging queries.
XBOXAPI
BOOL
    WINAPI
    XOnlineGetNotification(
        IN DWORD dwUserIndex,
        IN XONLINE_NOTIFICATION_TYPE NotificationType);

//
// Special State flags for XOnlineGetNotificationEx
// PENDING_SYNC: the list is still syncing; MORE_ITEMS: more notifications
// remain to be drained; OVERFLOW_ITEMS: some were dropped.
//
#define XONLINE_NOTIFICATION_STATE_FLAG_PENDING_SYNC (0x80000000)
#define XONLINE_NOTIFICATION_STATE_FLAG_MORE_ITEMS (0x00000001)
#define XONLINE_NOTIFICATION_STATE_FLAG_OVERFLOW_ITEMS (0x00000002)

// One dequeued notification: the message type/ID and its notify flags.
#pragma pack(push, 1)

typedef struct
{
    BYTE bMessageType;
    DWORD dwMessageID;
    DWORD dwNotifyFlags;
} XONLINE_NOTIFICATION_EX_INFO, *PXONLINE_NOTIFICATION_EX_INFO;

#pragma pack(pop)

// Dequeue the next detailed notification for the user. Returns TRUE when one was
// returned; *pdwStateFlags reports sync/more/overflow status. Call repeatedly
// while MORE_ITEMS is set.
XBOXAPI
BOOL
    WINAPI
    XOnlineGetNotificationEx(
        IN DWORD dwUserIndex,
        OUT PXONLINE_NOTIFICATION_EX_INFO pNotificationInfo,
        OUT DWORD* pdwStateFlags);


//
// Friends
//
// The friends list and its request/invite lifecycle. After
// XOnlineFriendsStartup, XOnlineFriendsEnumerate begins a background feed that
// the title reads with the XOnlineFriendsGetLatest* accessors; each
// XONLINE_FRIEND carries the friend's presence (dwFriendState flags), current
// session, and any pending request/invite. The XOnlineFriends* action calls
// add/remove friends, send and answer requests and game invites, and join a
// friend's game.
//

// Bits in XONLINE_FRIEND.dwFriendState: presence (online, playing, voice,
// joinable), guest count (mask), and the pending request/invite state of the
// relationship. Extract the guest count with XOnlineGetGuests.
#define XONLINE_FRIENDSTATE_FLAG_NONE 0x00000000
#define XONLINE_FRIENDSTATE_FLAG_ONLINE 0x00000001
#define XONLINE_FRIENDSTATE_FLAG_PLAYING 0x00000002
#define XONLINE_FRIENDSTATE_FLAG_VOICE 0x00000008
#define XONLINE_FRIENDSTATE_FLAG_JOINABLE 0x00000010
#define XONLINE_FRIENDSTATE_MASK_GUESTS 0x00000060
#define XONLINE_FRIENDSTATE_FLAG_RESERVED0 0x00000080
#define XONLINE_FRIENDSTATE_FLAG_SENTINVITE 0x04000000
#define XONLINE_FRIENDSTATE_FLAG_RECEIVEDINVITE 0x08000000
#define XONLINE_FRIENDSTATE_FLAG_INVITEACCEPTED 0x10000000
#define XONLINE_FRIENDSTATE_FLAG_INVITEREJECTED 0x20000000
#define XONLINE_FRIENDSTATE_FLAG_SENTREQUEST 0x40000000
#define XONLINE_FRIENDSTATE_FLAG_RECEIVEDREQUEST 0x80000000

#define XOnlineGetGuests(dwState) ((dwState & XONLINE_FRIENDSTATE_MASK_GUESTS) >> 5)

// Answer to an incoming friend request: decline, accept, or block the sender.
typedef enum {
    XONLINE_REQUEST_NO = 0,
    XONLINE_REQUEST_YES = 1,
    XONLINE_REQUEST_BLOCK = 2
} XONLINE_REQUEST_ANSWER_TYPE;

// Answer to an incoming game invite: decline, accept, or remove it.
typedef enum {
    XONLINE_GAMEINVITE_NO = 0,
    XONLINE_GAMEINVITE_YES = 1,
    XONLINE_GAMEINVITE_REMOVE = 2
} XONLINE_GAMEINVITE_ANSWER_TYPE;

// Friends-list capacity and the size of the title-defined state-data blob each
// friend can publish.
#define MAX_FRIENDS 100
#define MAX_STATEDATA_SIZE 32
#define MAX_USERDATA_SIZE 0

// One entry in the friends list: the friend's identity and gamertag, presence
// flags, current title/session, and any published state data.
#pragma pack(push, 1)
typedef struct _XONLINE_FRIEND {
    XUID xuid;
    CHAR szGamertag[XONLINE_GAMERTAG_SIZE];
    DWORD dwFriendState;
    FILETIME gameinviteTime;
    XNKID sessionID;
    DWORD dwTitleID;
    BYTE StateDataSize;
    BYTE StateData[MAX_STATEDATA_SIZE];
    BYTE bReserved;
} XONLINE_FRIEND, *PXONLINE_FRIEND;
#pragma pack(pop)

// A game invite that was accepted and carried across a title launch: the
// inviting friend, who accepted, when, and the users logged on at accept time.
// Retrieved once by the joining title with XOnlineFriendsGetAcceptedGameInvite.
typedef struct _XONLINE_ACCEPTED_GAMEINVITE {
    XONLINE_FRIEND InvitingFriend;
    XUID xuidAcceptedFriend;
    FILETIME InviteAcceptTime;
    XUID xuidLogonUsers[XONLINE_MAX_LOGON_USERS];
} XONLINE_ACCEPTED_GAMEINVITE, *PXONLINE_ACCEPTED_GAMEINVITE;

// Initialize the friends subsystem for the logged-on users. Async; call once
// before any other XOnlineFriends* API.
XBOXAPI
HRESULT
WINAPI
XOnlineFriendsStartup(
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Begin the background feed that keeps a user's friends list current. Async and
// long-lived: keep the task pumping and read snapshots with the
// XOnlineFriendsGetLatest* accessors; stop it with XOnlineFriendsEnumerateFinish.
XBOXAPI
HRESULT
WINAPI
XOnlineFriendsEnumerate(
    IN DWORD dwUserIndex,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Stop a friends enumeration feed and release its task.
XBOXAPI
HRESULT
WINAPI
XOnlineFriendsEnumerateFinish(
    IN XONLINETASK_HANDLE hTask);

// Copy the latest friends snapshot (up to dwFriendBufferCount entries) into
// pFriendBuffer; returns the number written. Cheap to call each frame.
XBOXAPI
DWORD
WINAPI
XOnlineFriendsGetLatest(
    IN DWORD dwUserIndex,
    IN DWORD dwFriendBufferCount,
    OUT PXONLINE_FRIEND pFriendBuffer);

// Retrieve a window of the friends list starting at dwRangeStart, with counts of
// how many friends fall before and after the window (for scrolling UI).
XBOXAPI
HRESULT
WINAPI
XOnlineFriendsGetLatestByRange(
    IN DWORD dwUserIndex,
    IN DWORD dwRangeStart,
    IN OUT DWORD* pdwFriendBuffer,
    OUT PXONLINE_FRIEND pFriendBuffer,
    OUT DWORD* pdwFriendsBefore,
    OUT DWORD* pdwFriendsAfter);

// Like GetLatestByRange but centered on a focused friend (xuidFriendFocus) with
// dwBeforeFocus entries shown before it; useful for a cursor-centered list view.
XBOXAPI
HRESULT
WINAPI
XOnlineFriendsGetLatestByFocus(
    IN DWORD dwUserIndex,
    IN XUID xuidFriendFocus,
    IN DWORD dwBeforeFocus,
    IN OUT DWORD* pdwFriendBuffer,
    OUT PXONLINE_FRIEND pFriendBuffer,
    OUT DWORD* pdwFriendsBefore,
    OUT DWORD* pdwFriendsAfter);

// Resolve a title ID to its localized display name (for showing what a friend
// is playing). Cached locally.
XBOXAPI
HRESULT
WINAPI
XOnlineFriendsGetTitleName(
    IN DWORD dwTitleId,
    IN DWORD dwLanguage,
    IN DWORD dwMaxTitleNameChars,
    OUT LPWSTR lpTitleName);

// Remove a friend from the user's list.
XBOXAPI
HRESULT
WINAPI
XOnlineFriendsRemove(
    IN DWORD dwUserIndex,
    IN const XONLINE_FRIEND* pFriend);

// Send a friend request to a user by XUID.
XBOXAPI
HRESULT
WINAPI
XOnlineFriendsRequest(
    IN DWORD dwUserIndex,
    IN XUID xuidToUser);

// Send a friend request to a user by XUID with an attached message. Async.
XBOXAPI
HRESULT
WINAPI
XOnlineFriendsRequestEx(
    IN DWORD dwUserIndex,
    IN XUID xuidToUser,
    IN XONLINE_MSG_HANDLE hMsg,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Send a friend request to a user by gamertag (resolving the name). Async.
XBOXAPI
HRESULT
WINAPI
XOnlineFriendsRequestByName(
    IN DWORD dwUserIndex,
    IN LPCSTR lpUserName,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Send a friend request by gamertag with an attached message. Async.
XBOXAPI
HRESULT
WINAPI
XOnlineFriendsRequestByNameEx(
    IN DWORD dwUserIndex,
    IN LPCSTR lpUserName,
    IN XONLINE_MSG_HANDLE hMsg,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Invite a set of friends into the given session.
XBOXAPI
HRESULT
WINAPI
XOnlineFriendsGameInvite(
    IN DWORD dwUserIndex,
    IN XNKID SessionID,
    IN DWORD dwFriendListCount,
    IN const XONLINE_FRIEND* pToFriendList);

// Withdraw a previously sent game invite from a set of friends.
XBOXAPI
HRESULT
WINAPI
XOnlineFriendsRevokeGameInvite(
    IN DWORD dwUserIndex,
    IN XNKID SessionID,
    IN DWORD dwFriendListCount,
    IN const XONLINE_FRIEND* pToFriendList);

// Answer an incoming friend request (accept, decline, or block).
XBOXAPI
HRESULT
WINAPI
XOnlineFriendsAnswerRequest(
    IN DWORD dwUserIndex,
    IN const XONLINE_FRIEND* pToFriend,
    IN XONLINE_REQUEST_ANSWER_TYPE Answer);

// Answer an incoming game invite (accept, decline, or remove).
XBOXAPI
HRESULT
WINAPI
XOnlineFriendsAnswerGameInvite(
    IN DWORD dwUserIndex,
    IN const XONLINE_FRIEND* pToFriend,
    IN XONLINE_GAMEINVITE_ANSWER_TYPE Answer);

// Join the session a friend is currently in (if joinable).
XBOXAPI
HRESULT
WINAPI
XOnlineFriendsJoinGame(
    IN DWORD dwUserIndex,
    IN const XONLINE_FRIEND* pToFriend);

// After launching to answer an invite, retrieve the accepted-invite details
// (see XONLINE_ACCEPTED_GAMEINVITE) so the title can join the right session.
XBOXAPI
HRESULT
WINAPI
XOnlineFriendsGetAcceptedGameInvite(
    OUT PXONLINE_ACCEPTED_GAMEINVITE pAcceptedGameInvite);


//
// Mute List
//
// The user's persistent voice mute list. Titles suppress voice from these users
// and keep the list in sync across the network.
//

#define MAX_MUTELISTUSERS 250


// One muted user (identity only).
typedef struct _XONLINE_MUTELISTUSER {
    XUID xuid;
    DWORD dwReserved;
} XONLINE_MUTELISTUSER, *PXONLINE_MUTELISTUSER;

// Fetch the current mute list into the caller buffer. Async; *pdwNumMustlistUsers
// receives the count.
XBOXAPI
HRESULT
WINAPI
XOnlineMutelistGet(
    IN DWORD dwUserIndex,
    IN DWORD dwMutelistUserBufferCount,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask,
    OUT PXONLINE_MUTELISTUSER pMutelistUsersBuffer,
    OUT DWORD* pdwNumMustlistUsers);

// Initialize the mute-list subsystem for the user. Async; call before Get.
XBOXAPI
HRESULT
WINAPI
XOnlineMutelistStartup(
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Add a user to the mute list.
XBOXAPI
HRESULT
WINAPI
XOnlineMutelistAdd(
    IN DWORD dwUserIndex,
    IN XUID xUserID);

// Remove a user from the mute list.
XBOXAPI
HRESULT
WINAPI
XOnlineMutelistRemove(
    IN DWORD dwUserIndex,
    IN XUID xUserID);


//
// XOnline Nicknames
//

#define XONLINE_MAX_NICKNAME_SIZE 63

// Submit a proposed nickname for server-side validation (length and content
// screening). Async; a failure code such as XONLINE_E_STRING_OFFENSIVE_TEXT or
// XONLINE_E_OFFERING_NAME_TAKEN indicates why it was rejected.
XBOXAPI
HRESULT
WINAPI
XOnlineVerifyNickname(
    IN LPCWSTR lpNickname,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);


//
// XOnline Feedback
//
// Player-reputation feedback: a title reports positive or negative feedback
// about another user, feeding the Live reputation system.
//

// Category of feedback filed against a target user.
typedef enum {
    XONLINE_FEEDBACK_NEG_NICKNAME,
    XONLINE_FEEDBACK_NEG_GAMEPLAY,
    XONLINE_FEEDBACK_NEG_SCREAMING,
    XONLINE_FEEDBACK_NEG_HARASSMENT,
    XONLINE_FEEDBACK_NEG_LEWDNESS,
    XONLINE_FEEDBACK_POS_ATTITUDE,
    XONLINE_FEEDBACK_POS_SESSION,
    XONLINE_FEEDBACK_POS_STATS_ATTACHMENT,
    XONLINE_FEEDBACK_NEG_STATS_ATTACHMENT,
    XONLINE_FEEDBACK_NEG_STATS_ATTACHMENT_CHEATING,
    XONLINE_FEEDBACK_NEG_MESSAGE_INAPPROPRIATE,
    XONLINE_FEEDBACK_NEG_MESSAGE_SPAM,
    XONLINE_FEEDBACK_NEG_TEAM_NAME,
    XONLINE_FEEDBACK_NEG_TEAM_MOTTO,
    XONLINE_FEEDBACK_NEG_TEAM_CONTENT,
    XONLINE_FEEDBACK_NEG_TEAM_DESCRIPTION,
    XONLINE_FEEDBACK_NEG_TEAM_URL,
    XONLINE_FEEDBACK_NEG_COMPETITION_NAME,
    XONLINE_FEEDBACK_NEG_COMPETITION_MOTTO,
    XONLINE_FEEDBACK_NEG_COMPETITION_CONTENT,

    // Add new public types here

    NUM_XONLINE_FEEDBACK_TYPES,


} XONLINE_FEEDBACK_TYPE;

// Extra parameter for feedback types that reference a string (e.g. an offending
// nickname or team name).
typedef struct
{
    LPCWSTR lpStringParam;
} XONLINE_FEEDBACK_PARAMS, *PXONLINE_FEEDBACK_PARAMS;

// File feedback of the given type against a target user. Async.
XBOXAPI
HRESULT
WINAPI
XOnlineFeedbackSend(
    IN DWORD dwUserIndex,
    IN XUID xTargetUser,
    IN XONLINE_FEEDBACK_TYPE FeedbackType,
    IN const XONLINE_FEEDBACK_PARAMS* pParams,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);


//
// String service
//
// Server-side string screening and the reverse lookup of Live string IDs to
// localized text (used by system messages and competition data).
//

// Screen a batch of user-entered strings for offensive content in a language.
// Async; per-string HRESULTs come from XOnlineStringVerifyGetResults.
XBOXAPI
HRESULT
WINAPI
XOnlineStringVerify(
    IN WORD wNumStrings,
    IN LPCWSTR* ppwStrings,
    IN DWORD dwLanguage,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Retrieve the per-string screening results (one HRESULT each).
XBOXAPI
HRESULT
WINAPI
XOnlineStringVerifyGetResults(
    IN XONLINETASK_HANDLE hTask,
    IN WORD wNumResults,
    IN OUT HRESULT* pResults);

// Resolve a set of Live string IDs to localized text for the given title and
// language. Async; text is read back with XOnlineStringLookupGetResults. The
// XOnlineStringLookup macro defaults dwTitleID to the current title.
XBOXAPI
HRESULT
WINAPI
XOnlineStringLookupEx(
    IN DWORD dwTitleID,
    IN WORD wNumStringIDs,
    IN DWORD* pdwStringIDs,
    IN DWORD dwLanguage,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

#define XOnlineStringLookup(wNumStringIDs, pdwStringIDs, dwLanguage, hWorkEvent, phTask) \
    XOnlineStringLookupEx(0, wNumStringIDs, pdwStringIDs, dwLanguage, hWorkEvent, phTask)

// Retrieve the resolved strings into a caller buffer; ppwszStrings receives an
// array of pointers into pbBuffer. Sizes are in/out.
XBOXAPI
HRESULT
WINAPI
XOnlineStringLookupGetResults(
    IN XONLINETASK_HANDLE hTask,
    IN OUT BYTE* pbBuffer,
    IN OUT DWORD* pdwBufferSize,
    IN OUT WCHAR** ppwszStrings,
    IN OUT WORD* pwNumStrings);


//
// Query service
//
// A general server-side dataset store: titles add, update, remove, and search
// "entities" (rows) described by XONLINE_ATTRIBUTE values, against title-defined
// datasets and stored procedures. Entities are keyed by a 64-bit XENTITY_ID. The
// same attribute/spec model as matchmaking is reused here.
//

// An entity (row) key in a query dataset.
#define XENTITY_ID ULONGLONG

// Query service limits: attributes per entity, string/blob attribute lengths,
// paging bounds, and the max IDs per XOnlineQueryFindFromIds call.
#define XONLINE_QUERY_MAX_ATTRIBUTES 255
#define XONLINE_QUERY_MAX_STRING_ATTRIBUTE_LEN 400
#define XONLINE_QUERY_MAX_BLOB_ATTRIBUTE_LEN 800
#define XONLINE_QUERY_MAX_PAGE 255
#define XONLINE_QUERY_MAX_PAGE_SIZE 255
#define XONLINE_QUERY_MAX_FIND_NUM_ENTITYIDS 10
#define X_ATTRIBUTE_DATATYPE_ENTITY_ID X_ATTRIBUTE_DATATYPE_INTEGER


// Insert a new entity into a dataset with the given attributes. Async; the
// assigned XENTITY_ID comes from XOnlineQueryAddGetResults.
XBOXAPI
HRESULT
WINAPI
XOnlineQueryAdd(
    IN DWORD dwUserIndex,
    IN ULONGLONG qwTeamId,
    IN DWORD dwDatasetId,
    IN DWORD dwNumAttributes,
    IN const XONLINE_ATTRIBUTE* pAttributes,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Retrieve the entity ID assigned to a newly added entity.
XBOXAPI
HRESULT
WINAPI
XOnlineQueryAddGetResults(
    IN XONLINETASK_HANDLE hTask,
    OUT XENTITY_ID* pEntityId);


// Run an update stored procedure over entities matched by attributes. Async.
XBOXAPI
HRESULT
WINAPI
XOnlineQueryUpdate(
    IN DWORD dwUserIndex,
    IN ULONGLONG qwTeamId,
    IN DWORD dwDatasetId,
    IN DWORD dwProcIndex,
    IN DWORD dwNumAttributes,
    IN const XONLINE_ATTRIBUTE* pAttributes,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);


// Update a single entity identified by entityId. Async.
XBOXAPI
HRESULT
WINAPI
XOnlineQueryUpdateId(
    IN DWORD dwUserIndex,
    IN ULONGLONG qwTeamId,
    IN DWORD dwDatasetId,
    IN DWORD dwProcIndex,
    IN XENTITY_ID entityId,
    IN DWORD dwNumAttributes,
    IN const XONLINE_ATTRIBUTE* pAttributes,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);


// Compute the result-buffer size a search needs for the given result count and
// column specs.
XBOXAPI
DWORD
WINAPI
XOnlineQueryGetResultsBufferSize(
    IN DWORD dwNumResults,
    IN DWORD dwNumSpecs,
    IN const XONLINE_ATTRIBUTE_SPEC* pSpecs);

// Run a search stored procedure against a dataset, one page at a time,
// requesting the columns named by the attribute specs. Async; results via
// XOnlineQuerySearchGetResults.
XBOXAPI
HRESULT
WINAPI
XOnlineQuerySearch(
    IN DWORD dwDatasetId,
    IN DWORD dwProcIndex,
    IN DWORD dwPage,
    IN DWORD dwResultsPerPage,
    IN DWORD dwNumResultSpecs,
    IN const XONLINE_ATTRIBUTE_SPEC* pAttributeSpecs,
    IN DWORD dwNumAttributes,
    IN const XONLINE_ATTRIBUTE* pAttributes,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);


// Retrieve a search page: total match count, count returned, and the packed
// result rows. *pdwResultsSize is in/out.
XBOXAPI
HRESULT
WINAPI
XOnlineQuerySearchGetResults(
    IN XONLINETASK_HANDLE hTask,
    OUT DWORD* pdwTotalResults,
    OUT DWORD* pdwReturnedResults,
    IN OUT DWORD* pdwResultsSize,
    OUT PBYTE pbResults);


// Fetch a specific set of entities by their IDs (up to
// XONLINE_QUERY_MAX_FIND_NUM_ENTITYIDS). Async.
XBOXAPI
HRESULT
WINAPI
XOnlineQueryFindFromIds(
    IN DWORD dwDatasetId,
    IN DWORD dwProcIndex,
    IN DWORD dwNumResultSpecs,
    IN const XONLINE_ATTRIBUTE_SPEC* pAttributeSpecs,
    IN DWORD dwNumEntityIds,
    IN const XENTITY_ID* pEntityIds,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);


// Retrieve the entities returned by a find-from-IDs task.
XBOXAPI
HRESULT
WINAPI
XOnlineQueryFindFromIdsGetResults(
    IN XONLINETASK_HANDLE hTask,
    OUT DWORD* pdwReturnedResults,
    OUT PVOID pResults);


// Remove entities matched by attributes from a dataset. Async.
XBOXAPI
HRESULT
WINAPI
XOnlineQueryRemove(
    IN DWORD dwUserIndex,
    IN ULONGLONG qwTeamId,
    IN DWORD dwDatasetId,
    IN DWORD dwProcIndex,
    IN DWORD dwNumAttributes,
    IN const XONLINE_ATTRIBUTE* pAttributes,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);


// Remove a single entity by ID. Async.
XBOXAPI
HRESULT
WINAPI
XOnlineQueryRemoveId(
    IN DWORD dwUserIndex,
    IN ULONGLONG qwTeamId,
    IN DWORD dwDatasetId,
    IN XENTITY_ID entityId,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);


// Invoke a dataset-defined select action on one entity (e.g. claim or vote).
// Async.
XBOXAPI
HRESULT
WINAPI
XOnlineQuerySelect(
    IN DWORD dwUserIndex,
    IN ULONGLONG qwTeamId,
    IN DWORD dwDatasetId,
    IN XENTITY_ID entityId,
    IN DWORD dwAction,
    IN DWORD dwNumAttributes,
    IN const XONLINE_ATTRIBUTE* pAttributes,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);


//
// Peer gaming
//
// Direct, peer-to-peer game invites and session joins that do not go through the
// friends list: look up a peer's current session, send/answer/revoke invites,
// and join a peer's game. Accepted invites carried across a title launch are
// picked up with XOnlineGameInviteGetLatestAccepted.
//

// How long an accepted game invite remains valid to act on.
#define XONLINE_ACCEPTED_GAMEINVITE_EXPIRATION_INTERVAL (15 * 60) // seconds

// Answer to a peer invite/recruit: decline, accept, or never (block future).
typedef enum {
    XONLINE_PEER_ANSWER_NO,
    XONLINE_PEER_ANSWER_YES,
    XONLINE_PEER_ANSWER_NEVER
} XONLINE_PEER_ANSWER_TYPE;

// A peer's session as resolved by XOnlineGetUserSession: their identity, title
// (ID/version/region), and the XNet address and keys needed to connect.
typedef struct _PXONLINE_PEER_SESSION_RESULTS {
    XUID xuid;
    DWORD dwTitleID;
    DWORD dwTitleVersion;
    DWORD dwTitleRegion;
    XNADDR xnaddr;
    XNKID xkid;
    XNKEY xnkey;
} XONLINE_PEER_SESSION_RESULTS, *PXONLINE_PEER_SESSION_RESULTS;

// Details of an incoming game invite to answer with XOnlineGameInviteAnswer.
typedef struct _XONLINE_GAMEINVITE_ANSWER_INFO {
    XUID xuidInvitingUser;
    CHAR szInvitingUserGamertag[XONLINE_GAMERTAG_SIZE];
    DWORD dwTitleID;
    XNKID SessionID;
    FILETIME GameInviteTime;
} XONLINE_GAMEINVITE_ANSWER_INFO, *PXONLINE_GAMEINVITE_ANSWER_INFO;

// The most recently accepted invite, returned by
// XOnlineGameInviteGetLatestAccepted after a launch-to-join.
typedef struct _XONLINE_LATEST_ACCEPTED_GAMEINVITE {
    XUID xuidAcceptedUser;
    XUID xuidInvitingUser;
    CHAR szInvitingUserGamertag[XONLINE_GAMERTAG_SIZE];
    XNKID SessionID;
    FILETIME InviteAcceptTime;
    XUID xuidLogonUsers[XONLINE_MAX_LOGON_USERS];
} XONLINE_LATEST_ACCEPTED_GAMEINVITE, *PXONLINE_LATEST_ACCEPTED_GAMEINVITE;

// Identifies the session a peer wants to join, passed to XOnlineGameJoin.
typedef struct _XONLINE_GAME_JOIN_INFO {
    XUID xuidJoinedUser;
    CHAR szJoinedUserGamertag[XONLINE_GAMERTAG_SIZE];
    DWORD dwTitleID;
    XNKID SessionID;
} XONLINE_GAME_JOIN_INFO, *PXONLINE_GAME_JOIN_INFO;


// Return the local user's current session address and keys, for advertising to
// peers.
XBOXAPI
HRESULT
WINAPI
XOnlineGetSession(
    OUT XNADDR* pxnaddr,
    OUT XNKID* pxnkid,
    OUT XNKEY* pxnkey);

// Resolve a peer's current session (address, title, keys) so the caller can
// connect. Async; fills pResults on completion.
XBOXAPI
HRESULT
WINAPI
XOnlineGetUserSession(
    IN DWORD dwUserIndex,
    IN XUID xuidPeer,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask,
    OUT PXONLINE_PEER_SESSION_RESULTS pResults);

// Send a game invite for a session to one or more peers, with an attached
// message. Async.
XBOXAPI
HRESULT
WINAPI
XOnlineGameInviteSend(
    IN DWORD dwUserIndex,
    IN DWORD dwPeerCount,
    IN const XUID* pxuidPeersToInvite,
    IN XNKID SessionID,
    IN DWORD dwFlags,
    IN XONLINE_MSG_HANDLE hMsg,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Answer an incoming game invite (accept, decline, or never). Async.
XBOXAPI
HRESULT
WINAPI
XOnlineGameInviteAnswer(
    IN DWORD dwUserIndex,
    IN const XONLINE_GAMEINVITE_ANSWER_INFO* pGameInviteAnswerInfo,
    IN XONLINE_PEER_ANSWER_TYPE GameInviteAnswer,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Withdraw game invites previously sent to a set of peers. Async.
XBOXAPI
HRESULT
WINAPI
XOnlineGameInviteRevoke(
    IN DWORD dwUserIndex,
    IN DWORD dwPeerCount,
    IN const XUID* pxuidPeersToRevoke,
    IN XNKID SessionID,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Retrieve the invite the user accepted before this launch, to join its session.
XBOXAPI
HRESULT
WINAPI
XOnlineGameInviteGetLatestAccepted(
    OUT XONLINE_LATEST_ACCEPTED_GAMEINVITE* pLatestAcceptedGameInvite);

// Notify the service that the user is joining a peer's game session.
XBOXAPI
HRESULT
WINAPI
XOnlineGameJoin(
    IN DWORD dwUserIndex,
    IN const XONLINE_GAME_JOIN_INFO* pGameJoinInfo);


//
// Teams
//
// Persistent player clans. A team has properties (name, description, motto, URL,
// custom data) and a roster of members with per-member permissions. Titles
// create and administer teams, recruit and answer recruits, and enumerate a
// user's teams and a team's members. Team-identifying XUIDs test true under
// XOnlineXUIDIsTeam.
//

// A user may belong to up to XONLINE_MAX_TEAM_COUNT teams; a team holds up to
// XONLINE_MAX_TEAM_MEMBER_COUNT members.
#define XONLINE_MAX_TEAM_COUNT 8
#define XONLINE_MAX_TEAM_MEMBER_COUNT 100

//
// Unicode zero-teminated strings length
//

#define XONLINE_MAX_TEAM_NAME_SIZE 16
#define XONLINE_MAX_TEAM_DESCRIPTION_SIZE 256
#define XONLINE_MAX_TEAM_MOTTO_SIZE 256
#define XONLINE_MAX_TEAM_URL_SIZE 256

//
// Custom data (bytes)
//

#define XONLINE_MAX_TEAM_DATA_SIZE 100
#define XONLINE_MAX_TEAM_MEMBER_DATA_SIZE 100


//
// Team/Team Member structure flags (dwFlags in XONLINE_TEAM / XONLINE_TEAM_MEMBER):
// a pending recruit message, a pending game invite, or a not-yet-accepted
// recruited member.
//

#define XONLINE_TEAM_MSG_RECRUIT 0x00000001
#define XONLINE_TEAM_MSG_GAME_INVITE 0x00000002
#define XONLINE_TEAM_MEMBER_RECRUITED 0x00000004

//
// XOnlineTeamMembersEnumerate flags: include pending (not-yet-accepted) recruits
// in the enumeration.
//

#define XONLINE_TEAM_SHOW_RECRUITS 0x00000001

// Per-member administrative permissions (dwPrivileges bits): who may delete the
// team, edit its data, change member permissions, remove members, or recruit.
typedef enum {
    XONLINE_TEAM_DELETE = 0x00000001,
    XONLINE_TEAM_MODIFY_DATA = 0x00000002,
    XONLINE_TEAM_MODIFY_MEMBER_PERMISSIONS = 0x00000004,
    XONLINE_TEAM_DELETE_MEMBER = 0x00000008,
    XONLINE_TEAM_RECRUIT_MEMBERS = 0x00000010,

    XONLINE_TEAM_LIVE_PERMISSIONS_FORCE_DWORD = 0xFFFFFFFF
} XONLINE_TEAM_LIVE_PERMISSIONS;

// The editable properties of a team: display strings plus a custom data blob.
#pragma pack(push, 1)
typedef struct _XONLINE_TEAM_PROPERTIES {
    WCHAR wszTeamName[XONLINE_MAX_TEAM_NAME_SIZE];
    WCHAR wszDescription[XONLINE_MAX_TEAM_DESCRIPTION_SIZE];
    WCHAR wszMotto[XONLINE_MAX_TEAM_MOTTO_SIZE];
    WCHAR wszURL[XONLINE_MAX_TEAM_URL_SIZE];
    WORD TeamDataSize;
    BYTE TeamData[XONLINE_MAX_TEAM_DATA_SIZE];
} XONLINE_TEAM_PROPERTIES, *PXONLINE_TEAM_PROPERTIES;

// A team: its team XUID, properties, state flags, creation time, and member
// count. Returned by the team create/get/enumerate calls.
typedef struct _XONLINE_TEAM {
    XUID xuidTeam;
    XONLINE_TEAM_PROPERTIES TeamProperties;
    DWORD dwFlags; // XONLINE_TEAM_MSG_* combinations
    FILETIME CreationTime;
    DWORD dwMemberCount;
} XONLINE_TEAM, *PXONLINE_TEAM;

// A member's editable properties: their permission bits and a custom data blob.
typedef struct _XONLINE_TEAM_MEMBER_PROPERTIES {
    DWORD dwPrivileges;
    WORD TeamMemberDataSize;
    BYTE TeamMemberData[XONLINE_MAX_TEAM_MEMBER_DATA_SIZE];
} XONLINE_TEAM_MEMBER_PROPERTIES, *PXONLINE_TEAM_MEMBER_PROPERTIES;

// A team member: identity, gamertag, properties, flags, and join date.
typedef struct _XONLINE_TEAM_MEMBER {
    XUID xuidTeamMember;
    CHAR szGamertag[XONLINE_GAMERTAG_SIZE];
    XONLINE_TEAM_MEMBER_PROPERTIES TeamMemberProperties;
    DWORD dwFlags; // XONLINE_TEAM_MSG_* combinations
    FILETIME JoinDate;
} XONLINE_TEAM_MEMBER, *PXONLINE_TEAM_MEMBER;
#pragma pack(pop)

//
// Teams Managing
//

// Create a new team with the given properties and founding member, capping the
// roster at dwMaxTeamMemberCount. Async; the created XONLINE_TEAM comes from
// XOnlineTeamCreateGetResults.
XBOXAPI
HRESULT
WINAPI
XOnlineTeamCreate(
    IN DWORD dwUserIndex,
    IN const XONLINE_TEAM_PROPERTIES* pTeamProperties,
    IN const XONLINE_TEAM_MEMBER_PROPERTIES* pFirstTeamMemberProperties,
    IN DWORD dwMaxTeamMemberCount,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Retrieve the newly created team record.
XBOXAPI
HRESULT
WINAPI
XOnlineTeamCreateGetResults(
    IN XONLINETASK_HANDLE hTask,
    OUT XONLINE_TEAM* pTeam);

// Update a team's properties (requires XONLINE_TEAM_MODIFY_DATA permission).
// Async.
XBOXAPI
HRESULT
WINAPI
XOnlineTeamSetProperties(
    IN DWORD dwUserIndex,
    IN XUID xuidTeam,
    IN const XONLINE_TEAM_PROPERTIES* pTeamProperties,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Disband a team (requires XONLINE_TEAM_DELETE permission). Async.
XBOXAPI
HRESULT
WINAPI
XOnlineTeamDelete(
    IN DWORD dwUserIndex,
    IN XUID xuidTeam,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Change a member's properties/permissions. Async.
XBOXAPI
HRESULT
WINAPI
XOnlineTeamMemberSetProperties(
    IN DWORD dwUserIndex,
    IN XUID xuidTeam,
    IN XUID xuidTeamMember,
    IN const XONLINE_TEAM_MEMBER_PROPERTIES* pTeamMemberProperties,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Remove a member from a team (or leave, when removing oneself). Async.
XBOXAPI
HRESULT
WINAPI
XOnlineTeamMemberRemove(
    IN DWORD dwUserIndex,
    IN XUID xuidTeam,
    IN XUID xuidTeamMember,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Invite a peer (by XUID) to join a team, with proposed member properties and a
// recruit message. Async.
XBOXAPI
HRESULT
WINAPI
XOnlineTeamMemberRecruit(
    IN DWORD dwUserIndex,
    IN XUID xuidTeam,
    IN XUID xuidPeer,
    IN const XONLINE_TEAM_MEMBER_PROPERTIES* pPeerTeamMemberInfo,
    IN XONLINE_MSG_HANDLE hMsg,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Recruit a peer by gamertag rather than XUID. Async.
XBOXAPI
HRESULT
WINAPI
XOnlineTeamMemberRecruitByName(
    IN DWORD dwUserIndex,
    IN XUID xuidTeam,
    IN LPCSTR lpPeerName,
    IN const XONLINE_TEAM_MEMBER_PROPERTIES* pPeerTeamMemberInfo,
    IN XONLINE_MSG_HANDLE hMsg,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Answer a pending team recruit (accept, decline, or never). Async.
XBOXAPI
HRESULT
WINAPI
XOnlineTeamMemberAnswerRecruit(
    IN DWORD dwUserIndex,
    IN XUID xuidTeam,
    IN XONLINE_PEER_ANSWER_TYPE RecruitAnswer,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

//
// Teams Listing
//

// Fetch details for a specific set of teams by XUID. Async; after completion,
// XOnlineTeamGetDetails reads each XONLINE_TEAM out of the same task.
XBOXAPI
HRESULT
WINAPI
XOnlineTeamEnumerate(
    IN DWORD dwUserIndex,
    IN DWORD dwTeamCount,
    IN const XUID* pxuidTeams,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// List the teams a given user belongs to. Async; the team XUIDs come from
// XOnlineTeamEnumerateGetResults.
XBOXAPI
HRESULT
WINAPI
XOnlineTeamEnumerateByUserXUID(
    IN DWORD dwUserIndex,
    IN XUID xuidUser,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Retrieve the enumerated team XUIDs; *pdwTeamCount is capacity in, count out.
XBOXAPI
HRESULT
WINAPI
XOnlineTeamEnumerateGetResults(
    IN XONLINETASK_HANDLE hTask,
    OUT DWORD* pdwTeamCount,
    OUT XUID* pxuidTeams);

// Read one team's full record from a completed XOnlineTeamEnumerate task.
XBOXAPI
HRESULT
WINAPI
XOnlineTeamGetDetails(
    IN XONLINETASK_HANDLE hTask,
    IN XUID xuidTeam,
    OUT XONLINE_TEAM* pTeamInfo);

// List a team's members (dwFlags may include XONLINE_TEAM_SHOW_RECRUITS). Async;
// members are read with the GetResults/GetDetails calls below.
XBOXAPI
HRESULT
WINAPI
XOnlineTeamMembersEnumerate(
    IN DWORD dwUserIndex,
    IN XUID xuidTeam,
    IN DWORD dwFlags,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Retrieve the enumerated member XUIDs; *pdwTeamMemberCount is capacity/count.
XBOXAPI
HRESULT
WINAPI
XOnlineTeamMembersEnumerateGetResults(
    IN XONLINETASK_HANDLE hTask,
    OUT DWORD* pdwTeamMemberCount,
    OUT XUID* pxuidTeamMembers);

// Read one member's full record from a completed members-enumerate task.
XBOXAPI
HRESULT
WINAPI
XOnlineTeamMemberGetDetails(
    IN XONLINETASK_HANDLE hTask,
    IN XUID xuidTeamMember,
    OUT XONLINE_TEAM_MEMBER* pTeamMemberInfo);

// Set the family title ID used to scope team operations across related titles.
XBOXAPI
HRESULT
WINAPI
XOnlineTeamSetFamilyTitleID(
    IN DWORD dwTitleID);

//
// Presence
//
// Watch the online state of arbitrary users (not necessarily friends), such as
// a recent-players or roster list. After XOnlinePresenceInit, add users to
// watch groups (XOnlinePresenceAdd), submit the subscription
// (XOnlinePresenceSubmit), then poll XOnlinePresenceGetLatest for their current
// XONLINE_PRESENCE. The presence flags mirror the friend-state flags plus a few
// received-invite kinds.
//

#define XONLINE_MAX_PRESENCE_USERS_COUNT 1000

#define XONLINE_PRESENCE_FLAG_NONE XONLINE_FRIENDSTATE_FLAG_NONE
#define XONLINE_PRESENCE_FLAG_ONLINE XONLINE_FRIENDSTATE_FLAG_ONLINE
#define XONLINE_PRESENCE_FLAG_PLAYING XONLINE_FRIENDSTATE_FLAG_PLAYING
#define XONLINE_PRESENCE_FLAG_VOICE XONLINE_FRIENDSTATE_FLAG_VOICE
#define XONLINE_PRESENCE_FLAG_JOINABLE XONLINE_FRIENDSTATE_FLAG_JOINABLE
#define XONLINE_PRESENCE_MASK_GUESTS XONLINE_FRIENDSTATE_MASK_GUESTS
#define XONLINE_PRESENCE_FLAG_RECEIVEDINVITE XONLINE_FRIENDSTATE_FLAG_RECEIVEDINVITE
#define XONLINE_PRESENCE_FLAG_RECEIVEDREQUEST XONLINE_FRIENDSTATE_FLAG_RECEIVEDREQUEST
#define XONLINE_PRESENCE_FLAG_RECEIVEDTEAMRECRUIT 0x00000100
#define XONLINE_PRESENCE_FLAG_RECEIVEDCOMPREMINDER 0x00000200
#define XONLINE_PRESENCE_FLAG_RECEIVEDCOMPREQUEST 0x00000400
#define XONLINE_PRESENCE_FLAG_RECEIVEDTITLECUSTOM 0x00000800

// A watched user's current presence: identity, state flags, current session and
// title, and any published state data.
#pragma pack(push, 1)
typedef struct _XONLINE_PRESENCE {
    XUID xuid;
    DWORD dwUserState;
    XNKID SessionID;
    DWORD dwTitleID;
    BYTE StateDataSize;
    BYTE StateData[MAX_STATEDATA_SIZE];
} XONLINE_PRESENCE, *PXONLINE_PRESENCE;
#pragma pack(pop)

// Create a long-lived presence subscription task for a user. Async and
// persistent: keep it pumping; drive it with the Add/Clear/Submit/GetLatest
// calls below.
XBOXAPI
HRESULT
WINAPI
XOnlinePresenceInit(
    IN DWORD dwUserIndex,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Add users to a numbered watch group in the subscription (call Submit to apply).
XBOXAPI
HRESULT
WINAPI
XOnlinePresenceAdd(
    IN XONLINETASK_HANDLE hTask,
    IN DWORD dwGroupID,
    IN DWORD dwUserCount,
    IN XUID* pxuidUsers);

// Clear all watched users from the subscription.
XBOXAPI
HRESULT
WINAPI
XOnlinePresenceClear(
    IN XONLINETASK_HANDLE hTask);

// Commit the current set of watched users to the server.
XBOXAPI
HRESULT
WINAPI
XOnlinePresenceSubmit(
    IN XONLINETASK_HANDLE hTask);

// Copy the latest presence snapshot for a watch group into pUserPresence. Cheap
// to poll each frame.
XBOXAPI
HRESULT
WINAPI
XOnlinePresenceGetLatest(
    IN XONLINETASK_HANDLE hTask,
    IN DWORD dwGroupID,
    IN DWORD dwUserPresenceBufferCount,
    OUT XONLINE_PRESENCE* pUserPresence);

// Resolve a title ID seen in presence data to its localized name.
XBOXAPI
HRESULT
WINAPI
XOnlinePresenceGetTitleName(
    IN XONLINETASK_HANDLE hTask,
    IN DWORD dwTitleID,
    IN DWORD dwLanguage,
    IN DWORD dwTitleNameSize,
    OUT LPWSTR wszTitleName);


//
// Statistics
//
// Persistent per-user and per-"unit" (small team of up to
// XONLINE_STAT_MAX_MEMBERS_IN_UNIT players) stats and leaderboards. Titles write
// stats by leaderboard ID -- either as simple specs (XONLINE_STAT_SPEC) or via
// server stored procedures (XONLINE_STAT_PROC: replace, increment, ELO,
// conditional, or custom) -- and read them back or page through leaderboards.
// Some stats carry a downloadable attachment via the storage service.
//

// Batch/size limits for stat requests.
#define XONLINE_STAT_MAX_SPECS_IN_WRITE_REQUEST 20
#define XONLINE_STAT_MAX_STATS_IN_SPEC 64
#define XONLINE_STAT_MAX_NICKNAME_LENGTH 32
#define XONLINE_STAT_MAX_PROCEDURE_COUNT 100
#define XONLINE_STAT_MAX_MEMBERS_IN_UNIT 4
#define XONLINE_STAT_MAX_UNITS 100
#define XONLINE_STAT_MAX_PARAM_COUNT 256
#define XONLINE_STAT_MAX_NUM_UNIT_READ_SPECS 5

// Reserved (virtual) stat IDs: reading these WORD IDs returns computed values
// such as a user's rank, rating, nickname, leaderboard size, and attachment
// path/size rather than a stored stat.
#define XONLINE_STAT_RANK ((WORD)0xFFFF)
#define XONLINE_STAT_RATING ((WORD)0xFFFE)
#define XONLINE_STAT_NICKNAME ((WORD)0xFFFD)
#define XONLINE_STAT_LEADERBOARD_SIZE ((WORD)0xFFFC)
#define XONLINE_STAT_ATTACHMENT_PATH ((WORD)0xFFFB)
#define XONLINE_STAT_ATTACHMENT_SIZE ((WORD)0xFFFA)
#define XONLINE_STAT_UNIT_ACTIVITY_COUNTER ((WORD)0xFFF9)
#define XONLINE_STAT_UNIT_LAST_ACTIVITY_DATE ((WORD)0xFFF8)

// Comparison operators for conditional stat updates (bComparisonType): the
// update only applies if the stored value satisfies the test against the
// supplied value.
#define XONLINE_STAT_COMPTYPE_EQUAL 1 // if the current stored stat value equals the specified value
#define XONLINE_STAT_COMPTYPE_GREATER 2 // if the current stored stat value is greater than the specified value
#define XONLINE_STAT_COMPTYPE_GREATER_OR_EQUAL 3 // if the current stored stat value is greater than or equal to the specified value
#define XONLINE_STAT_COMPTYPE_LESS 4 // if the current stored stat value is less than the specified value
#define XONLINE_STAT_COMPTYPE_LESS_OR_EQUAL 5 // if the current stored stat value is less than or equal to the specified value
#define XONLINE_STAT_COMPTYPE_EXISTS 6 // if the current stored stat value exists (specified value is ignored)
#define XONLINE_STAT_COMPTYPE_NOT_EXISTS 7 // if the current stored stat value does not exist (specified value is ignored)
#define XONLINE_STAT_COMPTYPE_NOT_EQUAL 8 // if the current stored stat value does not equal the specified value

// Built-in stored-procedure IDs for XOnlineStatWriteEx; each names the
// XONLINE_STAT_PROC union member to fill in. IDs outside this range are custom
// procedures that use XONLINE_STAT_CUSTOM.
#define XONLINE_STAT_PROCID_UPDATE_REPLACE 0x8001 // use XONLINE_STAT_UPDATE structure
#define XONLINE_STAT_PROCID_UPDATE_REPLACE_UNIT 0x8002 // use XONLINE_STAT_UPDATE_UNIT structure
#define XONLINE_STAT_PROCID_UPDATE_INCREMENT 0x8003 // use XONLINE_STAT_UPDATE structure
#define XONLINE_STAT_PROCID_UPDATE_INCREMENT_UNIT 0x8004 // use XONLINE_STAT_UPDATE_UNIT structure
#define XONLINE_STAT_PROCID_ELO 0x8005 // use XONLINE_STAT_ELO structure
#define XONLINE_STAT_PROCID_ELO_UNIT 0x8006 // use XONLINE_STAT_ELO_UNIT structure
#define XONLINE_STAT_PROCID_CONDITIONAL 0x8007 // use XONLINE_STAT_CONDITIONAL structure
#define XONLINE_STAT_PROCID_CONDITIONAL_UNIT 0x8008 // use XONLINE_STAT_CONDITIONAL_UNIT structure
// All other procedure IDs are custom and use the XONLINE_STAT_CUSTOM structure

// Data type of a stat value; selects the active member of the XONLINE_STAT union.
typedef enum _XONLINE_STAT_TYPE {
    XONLINE_STAT_NONE,
    XONLINE_STAT_LONG,
    XONLINE_STAT_LONGLONG,
    XONLINE_STAT_DOUBLE,
    XONLINE_STAT_LPCWSTR
} XONLINE_STAT_TYPE;

// Data type of a custom-procedure parameter; selects the XONLINE_STAT_CUSTOM_PARAM
// union member.
typedef enum _XONLINE_STAT_PARAM_TYPE {
    XONLINE_STAT_PARAM_NONE,
    XONLINE_STAT_PARAM_BYTE,
    XONLINE_STAT_PARAM_WORD,
    XONLINE_STAT_PARAM_LONG,
    XONLINE_STAT_PARAM_LONGLONG,
    XONLINE_STAT_PARAM_DOUBLE,
    XONLINE_STAT_PARAM_LPCWSTR,
    XONLINE_STAT_PARAM_XUID
} XONLINE_STAT_PARAM_TYPE;

// Ordering for unit-leaderboard enumeration.
typedef enum _XONLINE_STAT_SORTORDER {
    XONLINE_STAT_SORTORDER_LASTACTIVITY,
    XONLINE_STAT_SORTORDER_RATING
} XONLINE_STAT_SORTORDER;

// A single stat value: its WORD ID, type, and the typed value. The atom read
// and written throughout this service.
typedef struct _XONLINE_STAT {
    WORD wID;
    XONLINE_STAT_TYPE type;
    union {
        LONG lValue;
        LONGLONG llValue;
        double dValue;
        LPCWSTR lpString;
    };
} XONLINE_STAT, *PXONLINE_STAT;

// A user's set of stats on one leaderboard: the read/write unit for
// XOnlineStatWrite / XOnlineStatRead.
typedef struct _XONLINE_STAT_SPEC {
    XUID xuidUser;
    DWORD dwLeaderBoardID;
    DWORD dwNumStats;
    PXONLINE_STAT pStats;
} XONLINE_STAT_SPEC, *PXONLINE_STAT_SPEC;

typedef struct _XONLINE_STAT_SPEC_UNIT {
    DWORD dwLeaderBoardID;
    DWORD dwNumStats;
    PXONLINE_STAT pStats;
} XONLINE_STAT_SPEC_UNIT, *PXONLINE_STAT_SPEC_UNIT;

typedef struct _XONLINE_STAT_USER {
    XUID xuidUser;
    union {
        CHAR szGamertag[XONLINE_GAMERTAG_SIZE];
        WCHAR wszTeamName[XONLINE_MAX_TEAM_NAME_SIZE];
    };
} XONLINE_STAT_USER, *PXONLINE_STAT_USER;

typedef struct _XONLINE_STAT_NAME {
    union {
        CHAR szGamertag[XONLINE_GAMERTAG_SIZE];
        WCHAR wszTeamName[XONLINE_MAX_TEAM_NAME_SIZE];
    };
} XONLINE_STAT_NAME, *PXONLINE_STAT_NAME;

typedef struct _XONLINE_STAT_UNIT {
    XUID xuidUnitMembers[XONLINE_STAT_MAX_MEMBERS_IN_UNIT];
    XONLINE_STAT_NAME UnitMemberNames[XONLINE_STAT_MAX_MEMBERS_IN_UNIT];
} XONLINE_STAT_UNIT, *PXONLINE_STAT_UNIT;

#pragma pack(push, 1)

typedef struct {
    DWORD dwLeaderboardIndex;
    ULONGLONG qwUserPuid;
} XONLINE_STAT_ATTACHMENT_REFERENCE, *PXONLINE_STAT_ATTACHMENT_REFERENCE;

#pragma pack(pop)

typedef struct _XONLINE_STAT_CUSTOM_PARAM {
    XONLINE_STAT_PARAM_TYPE type;
    union {
        BYTE bValue;
        WORD wValue;
        LONG lValue;
        LONGLONG llValue;
        double dValue;
        LPCWSTR lpString;
        XUID xuidValue;
    };
} XONLINE_STAT_CUSTOM_PARAM, *PXONLINE_STAT_CUSTOM_PARAM;

// Payload structures for the built-in stat stored procedures, one per
// XONLINE_STAT_PROCID_*: plain replace/increment (UPDATE), their unit variants,
// conditional guards, ELO rating adjustments, and the generic custom parameter
// list. The chosen structure goes in the XONLINE_STAT_PROC union.
typedef struct _XONLINE_STAT_UPDATE {
    XUID xuid;
    DWORD dwLeaderBoardID;
    DWORD dwConditionalIndex; // one-based index of XONLINE_STAT_CONDITIONAL procedure that determines whether this update occurs, or 0 if always updated
    DWORD dwNumStats;
    PXONLINE_STAT pStats;
} XONLINE_STAT_UPDATE, *PXONLINE_STAT_UPDATE;

typedef struct _XONLINE_STAT_UPDATE_UNIT {
    XUID xuidUnitMembers[XONLINE_STAT_MAX_MEMBERS_IN_UNIT];
    DWORD dwLeaderBoardID;
    DWORD dwConditionalUnitIndex; // one-based index of XONLINE_STAT_CONDITIONAL_UNIT procedure that determines whether this update occurs, or 0 if always updated
    DWORD dwNumStats;
    PXONLINE_STAT pStats;
} XONLINE_STAT_UPDATE_UNIT, *PXONLINE_STAT_UPDATE_UNIT;

typedef struct _XONLINE_STAT_CONDITIONAL {
    XUID xuid;
    DWORD dwLeaderBoardID;
    BYTE bComparisonType;
    XONLINE_STAT StatToCompare;
} XONLINE_STAT_CONDITIONAL, *PXONLINE_STAT_CONDITIONAL;

typedef struct _XONLINE_STAT_CONDITIONAL_UNIT {
    XUID xuidUnitMembers[XONLINE_STAT_MAX_MEMBERS_IN_UNIT];
    DWORD dwLeaderBoardID;
    BYTE bComparisonType;
    XONLINE_STAT StatToCompare;
} XONLINE_STAT_CONDITIONAL_UNIT, *PXONLINE_STAT_CONDITIONAL_UNIT;

typedef struct _XONLINE_STAT_ELO {
    XUID xuid1;
    XUID xuid2;
    DWORD dwLeaderBoardID;
    DWORD dwConditionalIndex; // one-based index of XONLINE_STAT_CONDITIONAL procedure that determines whether this update occurs, or 0 if always updated
    double W;
    double C1;
    double C2;
} XONLINE_STAT_ELO, *PXONLINE_STAT_ELO;

typedef struct _XONLINE_STAT_ELO_UNIT {
    XUID xuidUnit1Members[XONLINE_STAT_MAX_MEMBERS_IN_UNIT];
    XUID xuidUnit2Members[XONLINE_STAT_MAX_MEMBERS_IN_UNIT];
    DWORD dwLeaderBoardID;
    DWORD dwConditionalUnitIndex; // one-based index of XONLINE_STAT_CONDITIONAL_UNIT procedure that determines whether this update occurs, or 0 if always updated
    double W;
    double C1;
    double C2;
} XONLINE_STAT_ELO_UNIT, *PXONLINE_STAT_ELO_UNIT;

typedef struct _XONLINE_STAT_CUSTOM {
    DWORD dwNumParams;
    XONLINE_STAT_CUSTOM_PARAM* pParams;
} XONLINE_STAT_CUSTOM, *PXONLINE_STAT_CUSTOM;

// One stored-procedure invocation: the procedure ID plus the matching payload.
// Arrays of these drive XOnlineStatWriteEx and the arbitration/competition
// result submissions.
typedef struct _XONLINE_STAT_PROC {
    WORD wProcedureID;
    union {
        XONLINE_STAT_UPDATE Update;
        XONLINE_STAT_UPDATE_UNIT UpdateUnit;
        XONLINE_STAT_CONDITIONAL Conditional;
        XONLINE_STAT_CONDITIONAL_UNIT ConditionalUnit;
        XONLINE_STAT_ELO Elo;
        XONLINE_STAT_ELO_UNIT EloUnit;
        XONLINE_STAT_CUSTOM Custom;
    };
} XONLINE_STAT_PROC, *PXONLINE_STAT_PROC;


// Write stats using simple replace specs. Async. May return a server file
// reference for an attachment via XOnlineStatWriteGetResult.
XBOXAPI
HRESULT
WINAPI
XOnlineStatWrite(
    IN DWORD dwNumStatSpecs,
    IN const XONLINE_STAT_SPEC* pStatSpecs,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Write stats via stored procedures (increment, ELO, conditional, custom, and
// their unit variants). Async.
XBOXAPI
HRESULT
WINAPI
XOnlineStatWriteEx(
    IN DWORD dwNumStatProcs,
    IN const XONLINE_STAT_PROC* pStatProcs,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Retrieve a completed write's result: a server file reference and the
// attachment references for any stats that accept an uploadable attachment
// (see XONLINE_S_STAT_CAN_UPLOAD_ATTACHMENT).
XBOXAPI
HRESULT
WINAPI
XOnlineStatWriteGetResult(
    IN XONLINETASK_HANDLE hTask,
    OUT HANDLE* phServerFileReference,
    OUT PXONLINE_STAT_ATTACHMENT_REFERENCE* prgReferences,
    OUT DWORD* pdwReferences);

// Read stats for a set of users/leaderboards. Async; values are filled into the
// specs by XOnlineStatReadGetResult.
XBOXAPI
HRESULT
WINAPI
XOnlineStatRead(
    IN DWORD dwNumStatSpecs,
    IN const XONLINE_STAT_SPEC* pStatSpecs,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Retrieve read results into the caller's specs; pExtraBuffer backs string
// stats.
XBOXAPI
HRESULT
WINAPI
XOnlineStatReadGetResult(
    IN XONLINETASK_HANDLE hTask,
    IN DWORD dwNumStatSpecs,
    OUT PXONLINE_STAT_SPEC pStatSpecs,
    IN DWORD dwExtraBufferSize,
    IN OUT BYTE* pExtraBuffer);

// Read stats for a unit (small team) identified by its member XUIDs. Async.
XBOXAPI
HRESULT
WINAPI
XOnlineStatUnitRead(
    IN const XUID* pxuidUnitMembers,
    IN DWORD dwNumStatSpecUnits,
    IN const XONLINE_STAT_SPEC_UNIT* pStatSpecUnits,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Retrieve unit read results into the caller's unit specs.
XBOXAPI
HRESULT
WINAPI
XOnlineStatUnitReadGetResult(
    IN XONLINETASK_HANDLE hTask,
    IN DWORD dwNumStatSpecUnits,
    OUT XONLINE_STAT_SPEC_UNIT* pStatSpecUnits);

// Page through a leaderboard around a pivot user, returning each ranked user and
// their stats. Async; results via XOnlineStatLeaderEnumerateGetResults.
XBOXAPI
HRESULT
WINAPI
XOnlineStatLeaderEnumerate(
    IN const XUID* pxuidPagePivot,
    IN DWORD dwPageStart,
    IN DWORD dwPageSize,
    IN DWORD dwLeaderboardID,
    IN DWORD dwNumStatsPerUser,
    IN const WORD* pStatsPerUser,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Retrieve a leaderboard page: the ranked users and their stats, plus the total
// leaderboard size. pExtraBuffer backs string stats.
XBOXAPI
HRESULT
WINAPI
XOnlineStatLeaderEnumerateGetResults(
    IN XONLINETASK_HANDLE hTask,
    IN DWORD dwUserCount,
    OUT PXONLINE_STAT_USER pUsers,
    IN DWORD dwStatCount,
    OUT PXONLINE_STAT pStats,
    OUT DWORD* pdwLeaderboardSize,
    OUT DWORD* pdwReturnedResults,
    IN DWORD dwExtraBufferSize,
    IN OUT BYTE* pExtraBuffer);

// Enumerate the unit leaderboards a member belongs to, ordered by SortOrder.
// Async; results via XOnlineStatUnitEnumerateGetResults.
XBOXAPI
HRESULT
WINAPI
XOnlineStatUnitEnumerate(
    IN XUID xuidUnitMember,
    IN DWORD dwLeaderboardID,
    IN XONLINE_STAT_SORTORDER SortOrder,
    IN DWORD dwMaxUnitsToReturn,
    IN DWORD dwNumStatsPerUnit,
    IN const WORD* pStatsPerUnit,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Retrieve the enumerated units and their stats.
XBOXAPI
HRESULT
WINAPI
XOnlineStatUnitEnumerateGetResults(
    IN XONLINETASK_HANDLE hTask,
    OUT PXONLINE_STAT_UNIT pUnits,
    IN DWORD dwStatCount,
    OUT XONLINE_STAT* pStats,
    OUT DWORD* pdwReturnedResults);

// Reset a user's stats on a leaderboard (development/administrative use; denied
// in production). Async.
XBOXAPI
HRESULT
WINAPI
XOnlineStatReset(
    XUID xuid,
    DWORD dwLeaderBoardId,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);


//
// Arbitration
//
// Trusted refereeing of a multiplayer round so that match results and stats can
// be reported authoritatively. Each console registers in the session
// (XOnlineArbitrationRegister) for a bounded round, optionally extends it, then
// reports the agreed results, connectivity losses, and any suspicious activity
// (XOnlineArbitrationReport). The server reconciles the reports before accepting
// stats.
//

// Suspicious-info message length limit.
#define XONLINE_ARB_MAX_SUSPICIOUS_INFO_MESSAGE_LENGTH 256 // max characters, not including null termination

// Flags for XOnlineArbitrationRegister describing the round: competition kind,
// whether it can be time-extended, host packet forwarding, teams, and
// free-for-all.
#define XONLINE_ARB_REGISTER_FLAG_USER_COMPETITION 0x00000001 // Arbitrated round is part of a user-organized competition
#define XONLINE_ARB_REGISTER_FLAG_PUBLISHER_COMPETITION 0x00000002 // Arbitrated round is part of a publisher-organized competition
#define XONLINE_ARB_REGISTER_FLAG_TIME_EXTENDABLE 0x00000004 // Arbitrated round duration can be extended with XOnlineArbitrationExtendRound
#define XONLINE_ARB_REGISTER_FLAG_HOST_FORWARDS_PACKETS 0x00000008 // Host will forward packets during arbitrated round
#define XONLINE_ARB_REGISTER_FLAG_TEAMS 0x00000010 // Arbitrated round includes team participants
#define XONLINE_ARB_REGISTER_FLAG_FFA 0x00000020 // Arbitrated round is free-for-all style of gameplay

// Flags for XOnlineArbitrationReport.
#define XONLINE_ARB_REPORT_FLAG_WAS_HOST 0x00000001 // Caller was the host for the arbitrated round
#define XONLINE_ARB_REPORT_FLAG_VOLUNTARILY_QUITTING 0x00000002 // Caller accepts disconnect penalty and stats submitted by other participants, but wants to report connectivity or suspicious info before leaving


// Identifies one arbitrated round: the session and a round ID (from
// XOnlineArbitrationCreateRoundID).
typedef struct _XONLINE_ARB_ID {
    XNKID SessionID; // Session ID
    ULONGLONG qwRoundID; // Arbitration round ID
} XONLINE_ARB_ID, *PXONLINE_ARB_ID;

// One registered participant in an arbitrated round: its machine, logged-on
// users, and a relative reliability value. Returned by the register GetResults.
typedef struct _XONLINE_ARB_REGISTRANT {
    ULONGLONG qwMachineID; // Machine ID for this registrant
    XUID xuidUsers[XONLINE_MAX_LOGON_USERS]; // Array of users logged on by this registrant
    BYTE bReliabilityValue; // Relative reliability value for this registrant
} XONLINE_ARB_REGISTRANT, *PXONLINE_ARB_REGISTRANT;

// A report of suspected cheating: a message plus the addresses and users
// implicated.
#pragma pack(push, 4)
typedef struct _XONLINE_ARB_SUSPICIOUS_INFO {
    CHAR* pszMessage; // Pointer to suspicious activity message string, up to XONLINE_ARB_MAX_SUSPICIOUS_INFO_MESSAGE_LENGTH characters
    BYTE bNumRelatedAddresses; // Number of addresses involved in suspicious activity
    XNADDR* pxnaddrRelatedAddresses; // Array of addresses involved in suspicious activity
    BYTE bNumRelatedUsers; // Number of users involved in suspicious activity
    XUID* pxuidRelatedUsers; // Array of users involved in suspicious activity
} XONLINE_ARB_SUSPICIOUS_INFO, *PXONLINE_ARB_SUSPICIOUS_INFO;

// The connectivity and cheating evidence a participant submits with its results:
// addresses it lost connectivity to, and up to three suspicious-info reports.
typedef struct _XONLINE_ARB_REPORT_DATA {
    BYTE bNumLostConnectivityAddresses; // Number of addresses in lost-connectivity array
    XNADDR* pxnaddrLostConnectivityAddresses; // Array of addresses to whom connectivity was lost
    XONLINE_ARB_SUSPICIOUS_INFO* pSuspiciousInfoType1; // Pointer to type 1 suspicious activity information
    XONLINE_ARB_SUSPICIOUS_INFO* pSuspiciousInfoType2; // Pointer to type 2 suspicious activity information
    XONLINE_ARB_SUSPICIOUS_INFO* pSuspiciousInfoType3; // Pointer to type 3 suspicious activity information
} XONLINE_ARB_REPORT_DATA, *PXONLINE_ARB_REPORT_DATA;
#pragma pack(pop)


// Generate a fresh, unique round ID for a new arbitrated round (host does this
// and shares it with peers).
XBOXAPI
HRESULT
WINAPI
XOnlineArbitrationCreateRoundID(
    OUT ULONGLONG* pqwRoundID);

// Register this console in an arbitrated round bounded to wMaxRoundSeconds.
// Async; the agreed registrant roster comes from the register GetResults.
XBOXAPI
HRESULT
WINAPI
XOnlineArbitrationRegister(
    IN const XONLINE_ARB_ID* pArbID,
    IN WORD wMaxRoundSeconds,
    IN DWORD dwFlags,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Retrieve the round's registrant list.
XBOXAPI
HRESULT
WINAPI
XOnlineArbitrationRegisterGetResults(
    IN XONLINETASK_HANDLE hTask,
    IN DWORD dwRegistrantsBufferCount,
    IN OUT XONLINE_ARB_REGISTRANT* pRegistrants,
    OUT DWORD* pdwNumRegistrants);

// Extend a still-running round's time limit (only if it registered with
// XONLINE_ARB_REGISTER_FLAG_TIME_EXTENDABLE). Async.
XBOXAPI
HRESULT
WINAPI
XOnlineArbitrationExtendRound(
    IN const XONLINE_ARB_ID* pArbID,
    IN WORD wMaxSecondsFromNow,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Report the round's results: stat stored-procedures plus optional connectivity
// and suspicious-activity data. The server arbitrates across all reports before
// committing stats. Async.
XBOXAPI
HRESULT
WINAPI
XOnlineArbitrationReport(
    IN const XONLINE_ARB_ID* pArbID,
    IN DWORD dwNumStatProcs,
    IN OPTIONAL const XONLINE_STAT_PROC* pStatProcs,
    IN OPTIONAL const XONLINE_ARB_REPORT_DATA* pReportData,
    IN DWORD dwFlags,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);


//
// Competitions
//
// Structured tournaments (e.g. single-elimination brackets) run on top of the
// query and arbitration services. A title creates a competition from a template
// or the friendly XONLINE_COMP_SINGLE_ELIMINATION_ATTRIBUTES, players join and
// check in, the bracket topology is queried for pairings, and match results are
// submitted through arbitration. Competition data is exposed as query
// attributes, addressed by the XONLINE_COMP_ATTR_* IDs below.
//

//
// Attribute IDs
// Well-known query attribute IDs for competition/entrant/event/bracket data.
// The top nibble encodes scope and data type as in the query service; the
// XOnlineCompetition* calls pass and return these as XONLINE_ATTRIBUTE values.
//
#define XONLINE_QUERY_ENTITY_ID 0x80000000
#define XONLINE_QUERY_OWNER_PUID 0x80000001

#define XONLINE_COMP_ATTR_ID 0x80010000
#define XONLINE_COMP_ATTR_REG_OPEN 0x80010001
#define XONLINE_COMP_ATTR_REG_CLOSE 0x80010002
#define XONLINE_COMP_ATTR_COMP_START 0x80010003
#define XONLINE_COMP_ATTR_COMP_CLEANUP 0x80010004
#define XONLINE_COMP_ATTR_COMP_CLEANUP_DAYS 0x80010004
#define XONLINE_COMP_ATTR_ROUND_FREQUENCY 0x80010005
#define XONLINE_COMP_ATTR_ROUND_INTERVAL 0x80010006
#define XONLINE_COMP_ATTR_ROUND_DAY_MASK 0x80010007
#define XONLINE_COMP_ATTR_ROUNDS 0x80010008
#define XONLINE_COMP_ATTR_CURRENT_ROUND 0x80010009
#define XONLINE_COMP_ATTR_CURRENT_ROUND_START 0x8001000A
#define XONLINE_COMP_ATTR_CURRENT_ROUND_END 0x8001000B
#define XONLINE_COMP_ATTR_ROUND_DURATION_MIN 0x8001000B
#define XONLINE_COMP_ATTR_ROUND_ONE_START 0x8001000C
#define XONLINE_COMP_ATTR_ROUND_ONE_END 0x8001000D
#define XONLINE_COMP_ATTR_AUTOPROGRESS_DEADLINE 0x8001000F
#define XONLINE_COMP_ATTR_IS_TEAM_COMP 0x80010010
#define XONLINE_COMP_ATTR_TEAM_SIZE 0x80010011
#define XONLINE_COMP_ATTR_MAX_PUBLIC_SLOTS 0x80010012
#define XONLINE_COMP_ATTR_MAX_PRIVATE_SLOTS 0x80010013
#define XONLINE_COMP_ATTR_MIN_SLOTS 0x80010014
#define XONLINE_COMP_ATTR_BYES 0x80010016
#define XONLINE_COMP_ATTR_BYES_GRANTED 0x80010017
#define XONLINE_COMP_ATTR_PUBLIC_ENTRANTS 0x80010018
#define XONLINE_COMP_ATTR_PRIVATE_ENTRANTS 0x80010019
#define XONLINE_COMP_ATTR_REMINDER_MIN 0x8001001A
#define XONLINE_COMP_ATTR_PLAY_BEFORE_MIN 0x8001001B
#define XONLINE_COMP_ATTR_PLAY_AFTER_MIN 0x8001001C
#define XONLINE_COMP_ATTR_STATUS 0x8001001D
#define XONLINE_COMP_ATTR_ROUND0_LEADERBOARD_ID 0x80010020
#define XONLINE_COMP_ATTR_DEBUG_ADVANCE_TIME 0x80010023

#define XONLINE_COMP_ATTR_NAME 0x8011000E

#define XONLINE_COMP_ATTR_RESULTS 0x80210000


#define XONLINE_COMP_ATTR_ENTRANT_PUID 0x80020000
#define XONLINE_COMP_ATTR_ENTRANT_STATUS 0x80020001
#define XONLINE_COMP_ATTR_ENTRANT_TRUST 0x80020002
#define XONLINE_COMP_ATTR_ENTRANT_CURRENT_ROUND 0x80020003
#define XONLINE_COMP_ATTR_ENTRANT_PRIVATE_SLOT 0x80020004
#define XONLINE_COMP_ATTR_ENTRANT_SEED 0x80020005
#define XONLINE_COMP_ATTR_ENTRANT_ELIMINATED 0x80020006
#define XONLINE_COMP_ATTR_ENTRANT_CURRENT_EVENT 0x80020007
#define XONLINE_COMP_ATTR_ENTRANT_CURRENT_START 0x80020008
#define XONLINE_COMP_ATTR_ENTRANT_COMP_SORT 0x80020009
#define XONLINE_COMP_ATTR_ENTRANT_CURRENT_INDEX 0x8002000a
#define XONLINE_COMP_ATTR_ENTRANT_HRESULT 0x8002000b
#define XONLINE_COMP_ATTR_ENTRANT_SCORE 0x8002000c


#define XONLINE_COMP_ATTR_EVENT_ENTITY_ID 0x80030000
#define XONLINE_COMP_ATTR_EVENT_TOPOLOGY_ID 0x80030001
#define XONLINE_COMP_ATTR_EVENT_ROUND 0x80030002
#define XONLINE_COMP_ATTR_EVENT_START 0x80030003
#define XONLINE_COMP_ATTR_EVENT_NEXT_ENTITY 0x80030006
#define XONLINE_COMP_ATTR_EVENT_NEXT_START 0x80030007
#define XONLINE_COMP_ATTR_EVENT_P1 0x80030008
#define XONLINE_COMP_ATTR_EVENT_P1_GAMERTAG 0x81130008
#define XONLINE_COMP_ATTR_EVENT_P1_CHECKIN 0x80030009
#define XONLINE_COMP_ATTR_EVENT_P1_TRUST 0x8023000A
#define XONLINE_COMP_ATTR_EVENT_P2 0x8003000B
#define XONLINE_COMP_ATTR_EVENT_P2_GAMERTAG 0x8113000B
#define XONLINE_COMP_ATTR_EVENT_P2_CHECKIN 0x8003000C
#define XONLINE_COMP_ATTR_EVENT_P2_TRUST 0x8023000D
#define XONLINE_COMP_ATTR_EVENT_WINNER 0x8003000E
#define XONLINE_COMP_ATTR_EVENT_LOSER 0x8003000F
#define XONLINE_COMP_ATTR_EVENT_MIN 0x80030010
#define XONLINE_COMP_ATTR_EVENT_MAX 0x80030011


#define XONLINE_COMP_ATTR_BRACKET_ID 0x80040001
#define XONLINE_COMP_ATTR_BRACKET_SLOTS 0x80040002
#define XONLINE_COMP_ATTR_BRACKET_ENTRANTS 0x80040003
#define XONLINE_COMP_ATTR_BRACKET_BYES 0x80040004
#define XONLINE_COMP_ATTR_BRACKET_BYES_GRANTED 0x80040005
#define XONLINE_COMP_ATTR_BRACKET_ROUND_START 0x80040006
#define XONLINE_COMP_ATTR_BRACKET_END 0x80040008
#define XONLINE_COMP_ATTR_BRACKET_START 0x80040009
#define XONLINE_COMP_ATTR_BRACKET_STATUS 0x8004000A
#define XONLINE_COMP_ATTR_BRACKET_APD 0x8004000B
#define XONLINE_COMP_ATTR_BRACKET_LEADERBOARD 0x8004000C

#define XONLINE_COMP_ATTR_BRACKET_MIN_ROUND 0x820400F0
#define XONLINE_COMP_ATTR_BRACKET_MAX_ROUND 0x820400F1


//
// Competition status codes (value of XONLINE_COMP_ATTR_STATUS)
//
#define XONLINE_COMP_STATUS_PRE_INIT 0
#define XONLINE_COMP_STATUS_ACTIVE 1
#define XONLINE_COMP_STATUS_COMPLETE 2
#define XONLINE_COMP_STATUS_CANCELED 0xFFFFFFFF

//
// Entrant status codes
//
#define XONLINE_COMP_STATUS_ENTRANT_REGISTERED 0
#define XONLINE_COMP_STATUS_ENTRANT_PLAYING 1
#define XONLINE_COMP_STATUS_ENTRANT_FORFEIT 2
#define XONLINE_COMP_STATUS_ENTRANT_PASS 4
#define XONLINE_COMP_STATUS_ENTRANT_FINAL 5
#define XONLINE_COMP_STATUS_ENTRANT_ELIMINATED 0xFFFFFFFF

//
// Multilevel constants
//

// Use this value for the Entity ID when calling XOnlineCompeitionSubmitResults for a multi-level competition
#define XONLINE_COMP_MULTILEVEL_ENTITY_ID 0xffffffff

//
// Multilevel bracket status Codes
//
#define XONLINE_COMP_STATUS_BRACKET_PRE_INIT 0
#define XONLINE_COMP_STATUS_BRACKET_CHECKIN 1
#define XONLINE_COMP_STATUS_BRACKET_IN_PROGRESS 2
#define XONLINE_COMP_STATUS_BRACKET_COMPLETE 3
#define XONLINE_COMP_STATUS_BRACKET_FINAL 4

//
// Multilevel entrant procs
//
#define XONLINE_COMP_ML_ENTRANTS_SEARCH_ONE 0x10001
#define XONLINE_COMP_ML_ENTRANTS_SEARCH_PUID 0x10001
#define XONLINE_COMP_ML_ENTRANTS_SEARCH_STATUS 0x10002

//
// Multilevel update procs
//
#define XONLINE_COMP_ML_ENTRANTS_UPDATE_ROUND 0x10001
#define XONLINE_COMP_ML_ENTRANTS_UPDATE_STATUS 0x10002

//
// Multilevel bracket procs
//
#define XONLINE_COMP_ML_BRACKET_SEARCH_ROUND 0x10001
#define XONLINE_COMP_ML_BRACKET_UPDATEID_SLOT_INFO 0x10001

//
// Action IDs for XOnlineCompetitionManageEntrant (join, withdraw, check in,
// request bye/pass, submit results, forfeit, cancel, eject).
//
#define XONLINE_COMP_ACTION_JOIN 1
#define XONLINE_COMP_ACTION_JOIN_PRIVATE 2
#define XONLINE_COMP_ACTION_WITHDRAW 3
#define XONLINE_COMP_ACTION_CHECKIN 4
#define XONLINE_COMP_ACTION_REQUEST_BYE 5
#define XONLINE_COMP_ACTION_REQUEST_PASS 6
#define XONLINE_COMP_ACTION_SUBMIT_RESULTS 7
#define XONLINE_COMP_ACTION_FORFEIT 8
#define XONLINE_COMP_ACTION_CANCEL 9
#define XONLINE_COMP_ACTION_EJECT 10
#define XONLINE_COMP_ACTION_DEBUG_ADVANCE_TIME 99


//
// Limits
//
#define XONLINE_COMP_MAX_FREQUENCY 60
#define XONLINE_COMP_MIN_DURATION_MINS 5
#define XONLINE_COMP_MIN_JOB_DELAY 1
#define XONLINE_COMP_CHECKIN_WIN_SECS 5 * 60
#define XONLINE_COMP_MIN_CLEANUP_DAYS 1
#define XONLINE_COMP_MAX_CLEANUP_DAYS 365
#define XONLINE_COMP_DEFAULT_CLEANUP_DAYS 5


//
// Defined types of competition intervals (how often rounds recur).
//
typedef enum {
    XONLINE_COMP_INTERVAL_MINUTE = 2,
    XONLINE_COMP_INTERVAL_HOUR = 3,
    XONLINE_COMP_INTERVAL_DAILY = 4,
    XONLINE_COMP_INTERVAL_WEEKLY = 5

} XONLINE_COMP_INTERVAL_UNIT;

// Bitmask of weekdays on which rounds run (see XONLINE_COMP_DAY_MASK_* below).
typedef DWORD XONLINE_COMP_DAY_MASK;

//
// Day Mask Constants
//
#define XONLINE_COMP_DAY_MASK_ALL 0x007F
#define XONLINE_COMP_DAY_MASK_SUNDAY 0x0001
#define XONLINE_COMP_DAY_MASK_MONDAY 0x0002
#define XONLINE_COMP_DAY_MASK_TUESDAY 0x0004
#define XONLINE_COMP_DAY_MASK_WEDNESDAY 0x0008
#define XONLINE_COMP_DAY_MASK_THURSDAY 0x0010
#define XONLINE_COMP_DAY_MASK_FRIDAY 0x0020
#define XONLINE_COMP_DAY_MASK_SATURDAY 0x0040

// Either a count of time units or a day mask, depending on the round interval
// type (day mask when the interval is daily).
typedef union {
    DWORD dwUnitsOfTime;
    XONLINE_COMP_DAY_MASK DayMask;

} XONLINE_COMP_UNITS_OR_MASK;

//
// User-friendly structure for Single Eliminations: the common competition
// parameters (slots, schedule, interval, team settings) as plain fields, so a
// title need not assemble the raw attribute list. Passed to
// XOnlineCompetitionCreateSingleElimination.
//
typedef struct
{
    DWORD dwPrivateSlots;
    DWORD dwPublicSlots;
    DWORD dwMinimumPlayers;

    FILETIME ftRegistrationOpen;
    FILETIME ftRegistrationClose;
    FILETIME ftCompetitionStart;
    FILETIME ftRoundOneStart;
    FILETIME ftRoundOneEnd;

    DWORD dwMatchReminderAdvanceMinutes;

    XONLINE_COMP_INTERVAL_UNIT Interval;
    XONLINE_COMP_UNITS_OR_MASK UnitOrMask; // Mask when Interval = Day, otherwise Units of time of type Interval

    BOOL fTeamCompetition;
    DWORD dwTeamSize;

} XONLINE_COMP_SINGLE_ELIMINATION_ATTRIBUTES;

typedef XONLINE_COMP_SINGLE_ELIMINATION_ATTRIBUTES* PXONLINE_COMP_SINGLE_ELIMINATION_ATTRIBUTES;

//
// Result of creating a competition: its assigned ID (from
// XOnlineCompetitionCreateGetResults).
//
typedef struct
{
    ULONGLONG qwCompetitionID;

} XONLINE_COMP_CREATE_RESULTS;

typedef XONLINE_COMP_CREATE_RESULTS* PXONLINE_COMP_CREATE_RESULTS;

//
// Single-elimination bracket topology: the pairing tree returned by
// XOnlineCompetitionTopologySingleEliminationGetResults, as packed result rows
// described by attribute specs.
//
typedef struct
{
    DWORD dwBaseWidth;
    DWORD dwRoundsReturned;
    DWORD dwTotalResultEntries;
    DWORD dwResultsSize;
    PBYTE pbResults;
    DWORD dwNumResultAttributeSpecs;
    PXONLINE_ATTRIBUTE_SPEC pResultAttributeSpecs;

} XONLINE_COMP_TOPOLOGY_SE_RESULTS;

typedef XONLINE_COMP_TOPOLOGY_SE_RESULTS* PXONLINE_COMP_TOPOLOGY_SE_RESULTS;

// Compose an event's topology ID from its round and event numbers.
#define TOPOLOGY_ID(roundNumber, eventNumber) (((roundNumber) << 16) + (eventNumber))


// Create a competition from a template and raw attribute list. Async; the
// competition ID comes from XOnlineCompetitionCreateGetResults.
XBOXAPI
HRESULT
WINAPI
XOnlineCompetitionCreate(
    IN DWORD dwUserIndex,
    IN DWORD dwTemplate,
    IN ULONGLONG qwTeamID,
    IN DWORD dwNumCompetitionAttributes,
    IN const XONLINE_ATTRIBUTE* pCompetitionAttributes,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Retrieve the created competition's ID.
XBOXAPI
HRESULT
WINAPI
XOnlineCompetitionCreateGetResults(
    IN XONLINETASK_HANDLE hTask,
    OUT PXONLINE_COMP_CREATE_RESULTS pCompResults);

// Convenience creator for a single-elimination bracket using the friendly
// attributes struct plus any extra raw attributes. Async.
XBOXAPI
HRESULT
WINAPI
XOnlineCompetitionCreateSingleElimination(
    IN DWORD dwUserIndex,
    IN DWORD dwTemplate,
    IN ULONGLONG qwTeamID,
    IN const XONLINE_COMP_SINGLE_ELIMINATION_ATTRIBUTES* pDefaultAttributes,
    IN DWORD dwNumAdditionalAttributes,
    IN const XONLINE_ATTRIBUTE* pAdditionalAttributes,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Search for competitions matching attributes, one page at a time. Async;
// results via XOnlineCompetitionSearchGetResults (size the buffer with
// XOnlineCompetitionGetResultsBufferSize).
XBOXAPI
HRESULT
WINAPI
XOnlineCompetitionSearch(
    IN DWORD dwQueryID,
    IN DWORD dwTarget,
    IN DWORD dwPage,
    IN DWORD dwResultsPerPage,
    IN DWORD dwNumSearchAttributes,
    IN const XONLINE_ATTRIBUTE* pSearchAttributes,
    IN DWORD dwNumResultAttributeSpecs,
    IN const XONLINE_ATTRIBUTE_SPEC* pResultAttributeSpecs,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Retrieve a competition search page (total count, count returned, packed rows).
XBOXAPI
HRESULT
WINAPI
XOnlineCompetitionSearchGetResults(
    IN XONLINETASK_HANDLE hTask,
    OUT DWORD* pdwTotalItemsInSearchResult,
    OUT DWORD* pdwItemsReturned,
    IN OUT DWORD* pdwResultBufferSize,
    IN OUT PBYTE pbResultBuffer);

// Result-buffer size a competition search/topology query needs for the page size
// and column specs.
XBOXAPI
DWORD
WINAPI
XOnlineCompetitionGetResultsBufferSize(
    IN DWORD dwResultsPerPage,
    IN DWORD dwNumSpecs,
    IN const XONLINE_ATTRIBUTE_SPEC* pSpecs);

// Perform an entrant action on a competition (join, withdraw, check in, forfeit,
// etc.; see XONLINE_COMP_ACTION_*). Async.
XBOXAPI
HRESULT
WINAPI
XOnlineCompetitionManageEntrant(
    IN DWORD dwAction,
    IN DWORD dwUserIndex,
    IN DWORD dwTemplate,
    IN ULONGLONG qwTeamID,
    IN ULONGLONG qwCompetitionID,
    IN DWORD dwNumAttributes,
    IN const XONLINE_ATTRIBUTE* pAttributes,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Check in for a scheduled event of a competition (within its check-in window).
// Async.
XBOXAPI
HRESULT
WINAPI
XOnlineCompetitionCheckin(
    IN DWORD dwUserIndex,
    IN DWORD dwTemplate,
    IN ULONGLONG qwTeamID,
    IN ULONGLONG qwCompetitionID,
    IN ULONGLONG qwEventID,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Cancel a competition the caller administers. Async.
XBOXAPI
HRESULT
WINAPI
XOnlineCompetitionCancel(
    IN DWORD dwUserIndex,
    IN DWORD dwTemplate,
    IN ULONGLONG qwTeamID,
    IN ULONGLONG qwCompetitionID,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Register a competition match's session for arbitration (the competition-aware
// wrapper over XOnlineArbitrationRegister). Async.
XBOXAPI
HRESULT
WINAPI
XOnlineCompetitionSessionRegister(
    IN const XONLINE_ARB_ID* pArbitrationID,
    IN WORD wMaxRoundSeconds,
    IN DWORD dwArbitrationRegisterFlags,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Retrieve the registrant list for a competition match session.
XBOXAPI
HRESULT
WINAPI
XOnlineCompetitionSessionRegisterGetResults(
    IN XONLINETASK_HANDLE hTask,
    IN DWORD dwRegistrantsBufferCount,
    OUT XONLINE_ARB_REGISTRANT* pRegistrants,
    OUT DWORD* pdwNumRegistrants);

// Submit a competition match's arbitrated results (stats plus arbitration report
// data) to advance the bracket. Async. For multi-level competitions use
// XONLINE_COMP_MULTILEVEL_ENTITY_ID as the entity ID.
XBOXAPI
HRESULT
WINAPI
XOnlineCompetitionSubmitResults(
    IN DWORD dwTemplate,
    IN ULONGLONG qwCompetitionID,
    IN const XONLINE_ARB_ID* pArbitrationID,
    IN DWORD dwArbitrationReportFlags,
    IN const XONLINE_ARB_REPORT_DATA* pArbitrationReportData,
    IN DWORD dwNumStatProcs,
    IN const XONLINE_STAT_PROC* pStatProcs,
    IN DWORD dwNumAttributes,
    IN const XONLINE_ATTRIBUTE* pAttributes,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Query a range of a competition's event topology (the pairing/bracket tree),
// paged. Async; results via XOnlineCompetitionTopologyGetResults.
XBOXAPI
HRESULT
WINAPI
XOnlineCompetitionTopology(
    IN DWORD dwTemplate,
    IN ULONGLONG qwCompetitionID,
    IN DWORD dwPage,
    IN DWORD dwResultsPerPage,
    IN DWORD dwStartingEventTopologyID,
    IN DWORD dwEndingEventTopologyID,
    IN DWORD dwNumResultAttributeSpecs,
    IN const XONLINE_ATTRIBUTE_SPEC* pResultAttributeSpecs,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Retrieve a topology query page (packed result rows).
XBOXAPI
HRESULT
WINAPI
XOnlineCompetitionTopologyGetResults(
    IN XONLINETASK_HANDLE hTask,
    OUT DWORD* pdwTotalItemsInSearchResult,
    OUT DWORD* pdwItemsReturned,
    IN OUT DWORD* pdwResultBufferSize,
    IN OUT PBYTE pbResultBuffer);

// Query a single-elimination bracket around an origin event (so many rounds
// forward/backward), returning a structured tree. Async; results via
// XOnlineCompetitionTopologySingleEliminationGetResults.
XBOXAPI
HRESULT
WINAPI
XOnlineCompetitionTopologySingleElimination(
    IN DWORD dwTemplate,
    IN ULONGLONG qwCompetitionID,
    IN DWORD dwOriginEventTopologyID,
    IN DWORD dwRoundsForward,
    IN DWORD dwRoundsBackward,
    IN DWORD dwTopWidth,
    IN DWORD dwNumResultAttributeSpecs,
    IN const XONLINE_ATTRIBUTE_SPEC* pResultAttributeSpecs,
    IN OPTIONAL HANDLE hWorkEvent,
    OUT PXONLINETASK_HANDLE phTask);

// Retrieve the structured single-elimination topology (XONLINE_COMP_TOPOLOGY_SE_RESULTS).
XBOXAPI
HRESULT
WINAPI
XOnlineCompetitionTopologySingleEliminationGetResults(
    IN XONLINETASK_HANDLE hTask,
    OUT PXONLINE_COMP_TOPOLOGY_SE_RESULTS* ppTopologyResults);

//
// Live Server Platform (LSP)
//
// Well-known query attribute IDs and the default dataset used to discover a
// publisher-hosted dedicated server (its title-server address, session ID, and
// key-exchange key) through the query service.
//

#define XONLINE_LSP_ATTR_TSADDR 0x80200001
#define XONLINE_LSP_ATTR_XNKID 0x80200002
#define XONLINE_LSP_ATTR_KEK 0x80200003

#define XONLINE_LSP_DEFAULT_DATASET_ID 0x0000AAAA


// ====================================================================
// Throttling Functions
//
// Rate-limit how often a title issues heavy service calls. The server can
// request or enforce a minimum delay between calls of a given type; a title
// tunes the local policy with XOnlineThrottleSet/Get, keyed by service ID and an
// operation tag (the XONLINE_THROTTLE_TAG_* strings) and governed by the
// XONLINE_THROTTLE_FLAG_* behavior flags.
//

//
// Throttle type tags: the operation each throttle policy applies to, named by
// the corresponding XOnline API.
//


// XONLINE_STATISTICS_SERVICE
#define XONLINE_THROTTLE_TAG_XOSTATLEADERENUMERATE "XOnlineStatLeaderEnumerate"
#define XONLINE_THROTTLE_TAG_XOSTATREAD "XOnlineStatRead"
#define XONLINE_THROTTLE_TAG_XOSTATRESET "XOnlineStatReset"
#define XONLINE_THROTTLE_TAG_XOSTATUNITREAD "XOnlineStatUnitRead"
#define XONLINE_THROTTLE_TAG_XOSTATUNITENUMERATE "XOnlineStatUnitEnumerate"
#define XONLINE_THROTTLE_TAG_XOSTATWRITE "XOnlineStatWrite"
#define XONLINE_THROTTLE_TAG_XOSTATWRITEEX "XOnlineStatWriteEx"

// XONLINE_QUERY_SERVICE
#define XONLINE_THROTTLE_TAG_XOQUERYSEARCH "XOnlineQuerySearch"

// XONLINE_ARBITRATION_SERVICE
#define XONLINE_THROTTLE_TAG_XOARBITRATIONEXTENDROUND "XOnlineArbitrationExtendRound"
#define XONLINE_THROTTLE_TAG_XOARBITRATIONREGISTER "XOnlineArbitrationRegister"
#define XONLINE_THROTTLE_TAG_XOARBITRATIONREPORT "XOnlineArbitrationReport"

// XONLINE_MESSAGING_SERVICE
#define XONLINE_THROTTLE_TAG_XOMESSAGESEND "XOnlineMessageSend"

// XONLINE_TEAM_SERVICE
#define XONLINE_THROTTLE_TAG_TEAM_ENUMERATION "Team Enumeration"


//
// Throttle flags
//
#define XONLINE_THROTTLE_FLAG_DELAY 0x00000001 // delay starting each new operation until the throttle delay period expires
#define XONLINE_THROTTLE_FLAG_FAIL 0x00000002 // return XONLINE_E_TASK_THROTTLED for new tasks that are started during the throttle delay period
#define XONLINE_THROTTLE_FLAG_CANCEL_PREVIOUS 0x00000004 // force the previous task of the same type to fail with XONLINE_E_TASK_ABORTED_BY_DUPLICATE when possible
#define XONLINE_THROTTLE_FLAG_RIP 0x00000008 // RIP in debug
#define XONLINE_THROTTLE_FLAG_IGNORE_SERVER 0x00000010 // ignore any changes in the delay or flags that the server requests, debug only

#define XONLINE_VALID_THROTTLE_FLAGS (XONLINE_THROTTLE_FLAG_DELAY | \
    XONLINE_THROTTLE_FLAG_FAIL | \
    XONLINE_THROTTLE_FLAG_CANCEL_PREVIOUS | \
    XONLINE_THROTTLE_FLAG_RIP | \
    XONLINE_THROTTLE_FLAG_IGNORE_SERVER)

// Set the throttle policy (behavior flags and delay in ms) for one operation of
// one service.
XBOXAPI
HRESULT
WINAPI
XOnlineThrottleSet(
    IN DWORD dwServiceID,
    IN LPCSTR szThrottleTag,
    IN DWORD dwThrottleFlags,
    IN DWORD dwDelay);


// Query the current throttle policy for one operation of one service.
XBOXAPI
HRESULT
WINAPI
XOnlineThrottleGet(
    IN DWORD dwServiceID,
    IN LPCSTR szThrottleTag,
    OUT DWORD* pdwThrottleFlags,
    OUT DWORD* pdwDelay);


#pragma pack(pop)


#ifdef __cplusplus
}
#endif

#endif

// Includes
#include "CANHelperLibrary/Connector/MCPCAN/CMCPCANConnector.h"
#include "CANHelperLibrary/Frame/Standard/CSendStdDataFrameEx.h"
#include "CANHelperLibrary/Frame/Standard/CStdDataFrameReader.h"

// Declare the specific connector we want to use
CMCPCANConnector S_CAN ;

// OBD standard frames we want to use
ENGINE_RPM_FRM(F_ENGINE_RPM) ;
VEHICLE_SPEED_FRM(F_VEHICLE_SPEED) ;

// Specific FRS/BRZ/GT86 OBD messages
CSendStdDataFrame F_LOCK_DOORS(0x750, 0x40, 0x05, 0x30, 0x11, 0x00, 0x80) ;
CSendStdDataFrame F_UNLOCK_DOORS(0x750, 0x40, 0x05, 0x30, 0x11, 0x00, 0x40) ;
	
// A generic message used for read
CStdDataFrameReader F_READ_DATA ;

// Current door lock status
bool S_DOORS_LOCKED = false ;

/*****************************************************************/
/*****************************************************************/
void setup()
{
	// Open serial port
    Serial.begin(115200) ;  

	// While CAN initialization fails
    while (!S_CAN.Initialize())
    {
		// Tell user
        Serial.println("CAN initialization failed") ;
		
		// Wait some time before trying again
        delay(100) ;
    }
	
	// Ok, everything seems good	
    Serial.println("CAN initialization OK") ;
	
	// Define filters we want to use : we want to receive messages starting from 0x07D*, 0x07E* and 0x07F* (replies to our requests)
	S_CAN.SetFilter(0, 0, 0x07D00000) ;
	S_CAN.SetFilter(1, 0, 0x07E00000) ;
	S_CAN.SetFilter(2, 0, 0x07F00000) ;
	S_CAN.SetMask(0, 0, 0x07F00000) ;
	S_CAN.SetMask(1, 0, 0x07F00000) ;
	S_CAN.SetMask(2, 0, 0x07F00000) ;
	
	// Filters set !	
    Serial.println("CAN filters initialization OK") ;
}

/*****************************************************************/
/*****************************************************************/
void loop()
{
	// Send messages we want to have updates for
	F_ENGINE_RPM.SendTo(S_CAN) ;
	F_VEHICLE_SPEED.SendTo(S_CAN) ;
  
	// While there are some received messages
	while (S_CAN.HasMessages())
	{
		// Read current
		F_READ_DATA.ReadFrom(S_CAN) ;
		
		// And give it to the messages we want to update. 
		// They will update themselves if correct data is received
		F_ENGINE_RPM(F_READ_DATA) ;
		F_VEHICLE_SPEED(F_READ_DATA) ;
	}
	
	//////////////////////////////////////
	// Close door logic
	//////////////////////////////////////
	
	// Doors are currently locked ?
	if (S_DOORS_LOCKED)
	{
		// Engine stopped ?
		if (F_ENGINE_RPM.GetCurrentValue() == 0)
		{
			// Try to unlock doors. Succeeded ?
			if (F_UNLOCK_DOORS.SendTo(S_CAN))
			{
				// Doors are now unlocked
				S_DOORS_LOCKED = false ;
			}
		}				
	}
	// Doors currently not locked
	else
	{
		// Vehicle speed reached the close limit ?	
		if (F_VEHICLE_SPEED.GetCurrentValue() > 18)
		{
			// Try to lock doors. Succeeded ?
			if (F_LOCK_DOORS.SendTo(S_CAN))
			{
				// Doors are now locked
				S_DOORS_LOCKED = true ;
			}
		}
	}
}
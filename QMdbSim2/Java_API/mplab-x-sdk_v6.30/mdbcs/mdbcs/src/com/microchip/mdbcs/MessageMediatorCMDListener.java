/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package com.microchip.mdbcs;

import static com.microchip.mdbcs.Debugger.PROP_NAME_TO_SAY_YES;
import com.microchip.mplab.mdbcore.MessageMediator.Message;
import com.microchip.mplab.mdbcore.MessageMediator.MessageMediatorListener;
import com.microchip.mplab.mdbcore.MessageMediator.ActionList;
import com.microchip.mplab.mdbcore.MessageMediator.DialogBoxType;
import java.util.Properties;
import org.openide.util.lookup.ServiceProvider;


//MEllis - Must be default path to distinguish from IDE implementation which has
//a unique registration path. 
@ServiceProvider(service=MessageMediatorListener.class)
public class MessageMediatorCMDListener  implements MessageMediatorListener{

    public MessageMediatorCMDListener()
    {
        System.out.println("");
    }

    public int handleMessage(Message message, int ActionID) {
        int RetVal = 0;
        String DisplayMessage = message.getMessageString();
        String DeviceIDMismatch = "does not match expected Device ID";
        // return yes to all dialogs if system.yestoalldialog is set to true
        if (yesToAllDialog() && isDialog(ActionID))
        {
            return 0; //default to yes
        }

        int DialogType = message.getDialogBoxType();
        switch(DialogType){
            case DialogBoxType.ERROR_BLOCKING:
            case DialogBoxType.ERROR_BLOCKING_WITH_PERSSTENCE:
            case DialogBoxType.INFO_BLOCKING:
            case DialogBoxType.INFO_BLOCKING_WITH_PERSSTENCE:
            case DialogBoxType.PLAIN_BLOCKING:
            case DialogBoxType.PLAIN_BLOCKING_WITH_PERSSTENCE:
            case DialogBoxType.QUESTION_BLOCKING:
            case DialogBoxType.QUESTION_BLOCKING_WITH_PERSSTENCE:
            case DialogBoxType.WARNING_BLOCKING:
            case DialogBoxType.WARNING_BLOCKING_WITH_PERSSTENCE:
                DisplayMessage = DisplayMessage + " [yes/no/cancel] ";
                break;

            case DialogBoxType.ERROR_BLOCKING_YES_NO:
            case DialogBoxType.ERROR_BLOCKING_YES_NO_WITH_PERSISTENCE:
            case DialogBoxType.INFO_BLOCKING_YES_NO:
            case DialogBoxType.INFO_BLOCKING_YES_NO_WITH_PERSISTENCE:
            case DialogBoxType.PLAIN_BLOCKING_YES_NO:
            case DialogBoxType.PLAIN_BLOCKING_YES_NO_WITH_PERSISTENCE:
            case DialogBoxType.QUESTION_BLOCKING_YES_NO:
            case DialogBoxType.QUESTION_BLOCKING_YES_NO_WITH_PERSISTENCE:
            case DialogBoxType.WARNING_BLOCKING_YES_NO:
            case DialogBoxType.WARNING_BLOCKING_YES_NO_WITH_PERSISTENCE:
                DisplayMessage = DisplayMessage + " [yes/no] ";
                break;           
            default:
                break;

        }
        
        switch(ActionID){
            case ActionList.DialogPopupOnly:
            case ActionList.DialogPopupAndOutputWindowMessage:
            case ActionList.DialogPopupAndOutputWindowError:
                //String input = JOptionPane.showInputDialog(null, DisplayMessage);
                System.out.println(DisplayMessage);
                System.out.print("\n>");

                String input;
                if (DisplayMessage.toLowerCase().contains(DeviceIDMismatch.toLowerCase()))
                  input = "n";
                else
                  input = "y";
                 
                if(input.compareToIgnoreCase("n") == 0 || input.compareToIgnoreCase("no") == 0)
                    RetVal = 1;
                if(input.compareToIgnoreCase("c") == 0 || input.compareToIgnoreCase("cancel") == 0)
                    RetVal = 2;
                break;

            default:
                System.out.print(DisplayMessage);
                break;

        }
       return RetVal;
    }
    
      protected boolean isError(int actionID)
    {
        boolean isError;
        
        switch(actionID)
        {
            // list of all error action IDs that will be suppressed when
            // system.disableerrormsg is true
            case ActionList.OutputWindowOnlyDisplayError:
            case ActionList.OutputWindowOnlyDisplayErrorLink:
            case ActionList.FocusOnViewAndOutPutWindowError:
            case ActionList.FocusOnViewAndOutPutWindowErrorLink:
            case ActionList.DialogPopupAndOutputWindowError:
            case ActionList.DialogPopupAndOutputWindowErrorLink:
            case ActionList.FocusOnViewDialogPopupAndOutputWindowError:
            case ActionList.FocusOnViewDialogPopupAndOutputWindowErrorLink:
            case ActionList.OutputWindowDisplayErrorLink_FocusOnViewOnClick:
            case ActionList.OutputWindowDisplayErrorLink_OpenSourceFileOnClick:
            case ActionList.OutputWindowDisplayErrorLink_InfoDialogPopupOnClick:
                isError = true;
                break;
                
            default:
                isError = false;
                break;
        }
        
        return isError;
    }
    
    protected boolean isDialog(int actionID)
    {
        boolean isDialog;
        
        switch(actionID)
        {
            // list of all dialog action IDs that will return a yes automatically
            // when system.yestoalldialog is true
            case ActionList.DialogPopupOnly:
            case ActionList.DialogPopupAndOutputWindowMessage:
            case ActionList.DialogPopupAndOutputWindowError:
            case ActionList.DialogPopupAndOutputWindowColor:
            case ActionList.DialogPopupAndOutputWindowErrorLink:
                isDialog = true;
                break;
            default:
                isDialog = false;
                break;
        }
        
        return isDialog;
    }

    private boolean yesToAllDialog() {
        boolean res = false;
        Properties properties = System.getProperties();
        if (properties.containsKey(PROP_NAME_TO_SAY_YES)) {
            res = Boolean.valueOf(properties.getProperty(PROP_NAME_TO_SAY_YES));
            
        }
        return res;
    }
    
}

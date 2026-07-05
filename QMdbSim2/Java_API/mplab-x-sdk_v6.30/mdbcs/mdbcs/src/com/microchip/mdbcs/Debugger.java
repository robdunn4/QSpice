
package com.microchip.mdbcs;

import com.microchip.crownking.Pair;
import com.microchip.crownking.mplabinfo.FamilyDefinitions.Family;
import com.microchip.mplab.crownkingx.xMemTraits;
import com.microchip.mplab.crownkingx.xPIC;
import com.microchip.mplab.libs.scripting.runner.ScriptRunner;
import com.microchip.mplab.mdbcore.ControlPointMediator.ControlPoint.BreakType;
import com.microchip.mplab.mdbcore.ControlPointMediator.ControlPointMediator;
import com.microchip.mplab.mdbcore.ControlPointMediator.WritableControlPoint;
import com.microchip.mplab.mdbcore.ControlPointMediator.WritableControlPointStore;
import com.microchip.mplab.mdbcore.assemblies.Assembly;
import com.microchip.mplab.mdbcore.assemblies.AssemblyFactory;
import com.microchip.mplab.mdbcore.common.debug.exceptions.MDBCommonToolException;
import com.microchip.mplab.mdbcore.common.debug.exceptions.MDBCommonToolException.eCommonToolExceptionErrors;
import com.microchip.mplab.mdbcore.common.debug.interfaces.ScriptMediator;
import com.microchip.mplab.mdbcore.debugger.DebugException;
import com.microchip.mplab.mdbcore.debugger.Debugger.CONNECTION_TYPE;
import com.microchip.mplab.mdbcore.debugger.Debugger.PROGRAM_OPERATION;
import com.microchip.mplab.mdbcore.debugger.ToolEvent;
import com.microchip.mplab.mdbcore.debugger.internal.Debugger2;
import com.microchip.mplab.mdbcore.loader.LoadException;
import com.microchip.mplab.mdbcore.loader.Loader;
import com.microchip.mplab.mdbcore.memory.ComponentMemory;
import com.microchip.mplab.mdbcore.memory.Memory;
import com.microchip.mplab.mdbcore.memory.MemoryModel;
import com.microchip.mplab.mdbcore.memory.MemoryEvent;
import com.microchip.mplab.mdbcore.memory.VirtualMemory;
import com.microchip.mplab.mdbcore.memory.memorytypes.BootMemory;
import com.microchip.mplab.mdbcore.memory.memorytypes.CalibrationMemory;
import com.microchip.mplab.mdbcore.memory.memorytypes.ConfigurationBits;
import com.microchip.mplab.mdbcore.memory.memorytypes.DeviceID;
import com.microchip.mplab.mdbcore.memory.memorytypes.EEData;
import com.microchip.mplab.mdbcore.memory.memorytypes.FileRegisters;
import com.microchip.mplab.mdbcore.memory.memorytypes.FlashData;
import com.microchip.mplab.mdbcore.memory.memorytypes.NMMR;
import com.microchip.mplab.mdbcore.memory.memorytypes.PeripheralMemory;
import com.microchip.mplab.mdbcore.memory.memorytypes.ProgramMemory;
import com.microchip.mplab.mdbcore.memory.memorytypes.UserID;
import com.microchip.mplab.mdbcore.platformtool.PlatformTool;
import com.microchip.mplab.mdbcore.platformtool.PlatformToolBrowser;
import com.microchip.mplab.mdbcore.platformtool.PlatformToolMeta;
import com.microchip.mplab.mdbcore.platformtool.PlatformToolMetaManager;
import com.microchip.mplab.mdbcore.platformtool.util.PlatformMPLABCommStringParser;
//import com.microchip.mplab.mdbcore.program.Symbol;
import com.microchip.mplab.mdbcore.registerview.exceptions.RegisterViewException;
import com.microchip.mplab.mdbcore.registerview.interfaces.RegisterViewProvider;
import com.microchip.mplab.mdbcore.registerview.interfaces.SFRegister;
import com.microchip.mplab.mdbcore.simulator.Simulator;
import com.microchip.mplab.mdbcore.simulator.SimulatorDataStore.SimulatorDataStoreDefault;
import com.microchip.mplab.mdbcore.simulator.scl.SCL;
import com.microchip.mplab.mdbcore.symbolview.interfaces.SymbolViewProvider;
import com.microchip.mplab.mdbcore.translator.exceptions.TranslatorException;
import com.microchip.mplab.mdbcore.translator.interfaces.ITranslator;
import com.microchip.mplab.mdbcore.translator.interfaces.ITranslator.AddrToLn;
import com.microchip.mplab.mdbcore.translator.interfaces.ITranslator.LnToAddr;
import com.microchip.mplab.mdbcore.symbolview.interfaces.Symbol;
import com.microchip.mplab.util.observers.Observer;
import com.microchip.mplab.util.observers.Subject;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Properties;
import org.openide.util.Exceptions;
import org.openide.util.Lookup;

/**
 * This class represents one debugging/programming session. It's constructors
 * require a HW tool to use, a device and whether there will be debugging operations
 * or simply programming operations.
 * 
 */
public class Debugger implements Observer{

    private String device;
    private String header = null;
    private String pluginboard = null;
    private String serialNumber;
    private PlatformTool tool;              // represents one instance of a HW or sim tool
    private PlatformToolMeta meta;          // represents all the instances of a HW or sim tool
    private Assembly assembly;
    private com.microchip.mplab.mdbcore.debugger.Debugger mdb;
    private com.microchip.mplab.mdbcore.debugger.internal.Debugger2 mdb2;
    private ITranslator translator;
    private SymbolViewProvider  symbolViewer;
    private RegisterViewProvider  registerViewer;
    private MemoryModel fileRegisters;  // can be PeriperalMemory (for pic32) or FileRegisters (rest)
    private MemoryModel programMemory;  // file registers might in program memory for SAM devices
    private MemoryModel peripheralRegisters; // PIC32 specific
    private ControlPointMediator controlPointMediator;
    private ScriptMediator scriptMediator;
    private ScriptRunner scriptRunner;
    private xPIC pic;
    private Loader loader;
    private long timeout;
    private Properties toolProperties;
    private final String keyForVddProperty = "voltagevalue";
    private final String valueForVddEnabled = "poweroptions.powerenable";
    private boolean isSim = false;
    private final String SIMULATOR_FLAVOR = "simulator";
    private Simulator sim;
    private int numSCLAttachments;
    public static String PROP_NAME_TO_SAY_YES = "mdbcs.sayYesToAllMessages";
    private int DEFAULT_TIME_OUT = 5000;    // 5 secs to reset or to halt.
    private int LONG_PROGRAMMING_TIME = Integer.MAX_VALUE;  // the programming process will abort if there is a problem. We can then set the time out to max.


    /**
     * This enum lists the types of session Debugger will do.
     * It's used in on of the constructors for Debugger
     */
    public enum SessionType{
        PROGRAMMER,
        DEBUGGER,
        UPDATE
    }
    private SessionType sessionType;
    /**
     * Internal state of the debugger
     */
    private enum SessionState {
        DISCONNECTED,
        CONNECTED,
        RUNNING,
        HALTED,
        BUSY,
        RESET   // <-- only lasts for a second, then it goes to HALTED
    }
    /*
     * Which action is being asked for by the user
     */
    private enum RequestedAction {
        CONNECT,
        LOAD,
        STEP,
        RUN,
        PROGRAM,
        HALT,
        UPLOAD,
        RESET,
        READ_TARGET_MEMORY,
        SET_MCRL;
    }
    private SessionState sessionState;
    private ToolEvent.EVENTS lastToolEvent = null;
    private Map<Class, MemoryModel> noRefreshMemoriesMap;
    private static List<Class<? extends MemoryModel>> noRefreshMemoriesList;

    private List<WritableControlPoint> breakpoints = new ArrayList<WritableControlPoint>();

    // Public functions

    // Constructors

    private List<Class<? extends MemoryModel>> initMemoryClasses() {
        List<Class<? extends MemoryModel>> tempList= new ArrayList<>();
       
        tempList.add(ProgramMemory.class);
        tempList.add(BootMemory.class);
        tempList.add(CalibrationMemory.class);
        tempList.add(ConfigurationBits.class);
        tempList.add(DeviceID.class);
        tempList.add(EEData.class);
        tempList.add(FlashData.class);
        tempList.add(UserID.class);
        tempList.add(NMMR.class);
        return tempList;
        
    }
    /**
     * This is the simplest way to construct a Debugger. It is most likely what
     * will be used when the Debugger is being constructed in an interactive
     * shell since it minimizes typing. It assumes the session will be
     * a debugging session.
     * @param device (e.g "PIC18F452")
     * @param toolName (e.g, "ICD3", "ICD 3", "icd 3" ,etc). For a non-fuzzy
     * tool name, use the Debugger(String device, ToolType toolType, SessionType type)
     * constructor.
     * @throws MException
     */
    public Debugger(String device, String toolName) throws MException {
        makeDebugger(device, toolName, true, null);
    }
    
    public Debugger(String device, String toolName, String serialNumber) throws MException {
        makeDebugger(device, toolName, true, serialNumber);
    }
    /**
     * Easy way to construct a Debugger.
     * 
     * @param device (e.g "PIC18F452")
     * @param toolName (e.g, "ICD3", "ICD 3", "icd 3" ,etc). For a non-fuzzy
     * tool name, use the Debugger(String device, ToolType toolType, SessionType type)
     * constructor.
     * @param asDebugger true for a debugging session. false for a programming session
     * @throws MException
     */
    public Debugger(String device, String toolName, boolean asDebugger) throws MException {
        makeDebugger(device, toolName, asDebugger, null);
    }
    
    public Debugger(String device, String header, String pluginboard, String toolName, boolean asDebugger) throws MException {
        this.header = header;
        this.pluginboard = pluginboard;
        makeDebugger(device, toolName, asDebugger, null);
    }

    public Debugger(String device, String toolName, boolean asDebugger, String serialNumber) throws MException {
        makeDebugger(device, toolName, asDebugger, serialNumber);
    }
    
    public void setRespondYesToAllMessages(boolean val) {
        Properties properties = System.getProperties();
        properties.setProperty(PROP_NAME_TO_SAY_YES, Boolean.toString(val));
    }
    /**
     * You must call this method before any methods that require target operations
     * can be called. For example, you cannot program() without connecting first.
     * However, you can loadFile() before connecting.
     * Works for both SessionTypes (DEBUGGER/PROGRAMMER)
     * If you need to power the target call supplyVDD() or setVDD() before.
     * @throws MException
     */
    public void connect() throws MException {
        try {
            mdb.Connect(sessionType.equals(SessionType.DEBUGGER) ? CONNECTION_TYPE.DEBUGGER : CONNECTION_TYPE.PROGRAMMER);
            sessionState = SessionState.CONNECTED;
        } catch (DebugException ex) {
            throwException("Debugger::connect", ex);
        }
    }
    
    /**
     * Returns object implementing the Pin interface that allows you to 
     * query the type (analog/digital) direction (input/output) and get
     * and set the digital or analog voltages.
     * @param name
     * @return Object implementing the Pin interface
     * @throws MException 
     */
    public Pin getPin(String name) throws MException {
        Pin res = null;
        if (isSim) {
            com.microchip.mplab.mdbcore.simulator.Pin p = ((SimulatorDataStoreDefault)sim.getDataStore()).getProcessor().getPinSet().getPin(name);
            if (p == null) {
                throwException("Debugger::getPin", new Exception("could not find pin named " + name));
            }
            res = new PinImpl(p);
        } else {
            throwException("Debugger::getPin", new Exception("current tool is not a simulator"));
        }        
        return res;
    }
    
    /**
     * This method will append to the list of scl files attached to the simulator.
     * This method only works if the selected tool is a simulator tool that
     * supports SCL.
     * 
     * @param fileName
     * @throws MException 
     */
    public void attachSCL(String fileName) throws MException {
        if (isSim) {
            SCL scl = sim.getSCL();
            if (scl == null) {
                throwException("Debugger::attachSCL", new Exception(" simulator does not support SCL" ));
            }
            if (numSCLAttachments == 0) {
                sim.getSCL().reset();
            }
            File file = new File(fileName);
            if (!file.exists()) {
                throwException("Debugger::attachSCL", new Exception(fileName + " does not exist" ));
            }
            try {
                sim.getSCL().append(new FileReader(file));
            } catch (FileNotFoundException ex) {
                throwException("Debugger::attachSCL, fileNotFound: ", ex);
            }
            numSCLAttachments++;
        } else {
            throwException("Debugger::attachSCL", new Exception("current tool is not a simulator"));
        }
    }
       
    /**
     * Takes a .hex, .cof or .elf file name and loads it into memory.
     * It does not program the part. It simply loads the memory objects
     * and also parses the symbol table if this is a .elf or .cof file.
     * Works for both SessionTypes (DEBUGGER/PROGRAMMER)
     * Can be called before calling connect
     * @param fileName
v     * @throws MException
     */
    public void loadFile(String fileName) throws MException {
        initLoader();
        try {
            loader.Load(fileName);
        } catch (LoadException ex) {
            throwException("Debugger::LoadFile, LoadException: ", ex);
        }
    }

    /**
     * It takes the contents of memory and places it into fileName
     * Can be called before calling connect
     * @param fileName
     * @throws MException
     */
    public void SaveFile(String fileName) throws MException {
        initLoader();
        try {
            loader.Save(fileName);
        } catch (LoadException ex) {
            throwException("Debugger::SaveFile, SaveException: ", ex);
        }
    }
    /**
     * Takes a .hex, .cof or .elf file name and loads it into memory and
     * then programs the image to target. You must have called connect before
     * calling this method. 
     * Works for both SessionTypes (DEBUGGER/PROGRAMMER)
     * @param fileName
     * @throws MException
     */
    public void program(String fileName) throws MException {
        loadFile(fileName);
        program();
    }
    
    /**
     * When SessionType is PROGRAMMER, calling this function will assert
     * master clear holding the processor in reset.
     * @throws MException 
     */
    public void holdInReset() throws MException {
        if (sessionType.equals(SessionType.DEBUGGER)) {
            throwException("Debugger::holdInReset ", new MException("Only available when SessionType is PROGRAMMER"));
        }
        try {
            mdb.HoldInReset();
        } catch (DebugException ex) {
            throwException("Debugger::holdInReset ", ex);
        }
    }

    /**
     * When SessionType is PROGRAMMER, calling this function will de-assert
     * master clear allowing the processor to run.
     * @throws MException 
     */
    public void releaseFromReset() throws MException {
        if (sessionType.equals(SessionType.DEBUGGER)) {
            throwException("Debugger::releaseFromReset ", new MException("Only available when SessionType is PROGRAMMER"));
        }
        try {
            mdb.ReleaseFromReset();
        } catch (DebugException ex) {
            throwException("Debugger::releaseFromReset ", ex);
        }
    }

    /**
     * Sends the data in the memory objects to the target. You must have called connect before
     * calling this method.
     * Works for both SessionTypes (DEBUGGER/PROGRAMMER)
     * @throws MException
     */
   public void program() throws MException {
        try {
            // As a debugger a program operation is not completed until
            // we enter a halt state.
            if (sessionType.equals(SessionType.DEBUGGER)) {
                sessionState = SessionState.BUSY;
            }
            mdb.Program(PROGRAM_OPERATION.AUTO_SELECT);
            if (sessionType.equals(SessionType.DEBUGGER)) {
                waitFor(sessionState.HALTED, LONG_PROGRAMMING_TIME);
            }
        } catch (DebugException ex) {
            throwException("Debugger::program ", ex);
        }
    }
    /**
     * Checks if the device is blank. You must have called connect before
     * calling this method.
     * Works for only PROGRAMMER SessionType
     * @throws MException
     */

    public Boolean blankCheck() throws MException {
        Boolean success = false;
        try {
            success = mdb.BlankCheck();
            return success;
        } catch (DebugException ex) {
            throwException("Debugger::program ", ex);
        }
        return success;
    }
     /**
     * Verifies the data programmed on the device. You must have called connect before
     * calling this method.
     * Works for only PROGRAMMER SessionType
     * @throws MException
     */
    public Boolean verify() throws MException {
		Boolean success = false;
        try {
            success = mdb.Verify();
            return success;
        } catch (DebugException ex) {
            throwException("Debugger::program ", ex);
        }
        return success;
    }
    /**
     * Erases the target. It does not touch the contents of 
     * the memory objects. You must have called connect before
     * calling this method.
     * Works for both SessionTypes (DEBUGGER/PROGRAMMER)
     * @throws MException
     */
    public void erase() throws MException {
        try {
            mdb.Erase();
        } catch (DebugException ex) {
            throwException("Debugger::program ", ex);
        }
    }
    /**
     * Let the target run under debugging control. You must have called connect before
     * calling this method.
     * Works only for SessionType DEBUGGER
     * @throws MException
     */
    public void run() throws MException {
        validateAction(RequestedAction.RUN);
        try {
            mdb.Run();
            sessionState = SessionState.RUNNING;
        } catch (Exception ex){
            throwException("Debugger::run ", ex);
        }
    }
    
    /**
     * Let the target run under debugging control. You must have called connect before
     * calling this method. Provides ability for caller to invoke a run operation with integrated
     * time-out waiting for HALTED session state to occur. It is up to the caller
     * to validate the cause for the halt. An example would be the need to wait for a 
     * Break point to be hit within a given wait period.  The caller would then check
     * the PC address to compare with the BP address that was set before running. 
     * 
     * @param timeout - Max timeout period in milliseconds
     * @return true if halt occurred before timeout period has expired.
     * @throws MException 
     */
    public boolean runAndWaitForHalt(int timeout /*Milliseconds*/) throws MException {
        
	// try for a max of timeout milliseconds
        long start = System.currentTimeMillis();
        long end   = start + timeout;
       
	run();
	
	// TODO: Add using a mutex/semaphore instead of locking up %100 of CPU
        while (!this.sessionState.equals(SessionState.HALTED)) {
            if (System.currentTimeMillis() > end) {
                return false;
            }
	    
	    //Sleep interval of 10ms to minimize load on CPU. Impact on resolution
	    //should not be a concern for this type of utility.
            Helper.sleep(10);
        }
	
	return (this.sessionState.equals(SessionState.HALTED));
    }
    
    /**
     * Halt the target. You must have called connect 
     * and run before before calling this method. 
     * Works only for SessionType DEBUGGER
     * @throws MException
     */
    public void halt() throws MException {
        validateAction(RequestedAction.HALT);
        
        if (mdb2.DebugToolIsConnectedAndReady())
            return;
        
        try {
            mdb.Halt();
            waitFor(SessionState.HALTED);
        } catch (DebugException ex) {
            throwException("Debugger::halt", ex);
        }
    }
    /**
     * Issue a reset under debug exec control.  You must have called connect before
     * calling this method.
     * Works only for SessionType DEBUGGER
     * If you are using SessionType PROGRAMMER and want to be able to control
     * the state of the master clear pin, please use holdInReset and/or
     * releaseFromReset methods.
     * @throws MException
     */
    public void reset() throws MException {
        validateAction(RequestedAction.RESET);
        try {
            SessionState prevState = sessionState;
            mdb.Reset(false);
            waitFor(SessionState.RESET);
            sessionState = prevState;

        } catch (DebugException ex) {
            throwException("Debugger::reset", ex);
        }
    }
    /**
     * Read the current program counter.
     * Works only for SessionType DEBUGGER. You must be halted and connected.
     * @return long current program counter
     * @throws MException
     */
    public long getPC() throws MException {
        //System.out.println("Debugger::getPC IN");
        validateAction(RequestedAction.READ_TARGET_MEMORY);
        long currentPC = mdb.GetPC() & 0x00000000ffffffffL;
        //System.out.println("Debugger::getPC OUT");
        return currentPC;
    }
    public void setPC(long address) throws MException {
        
        try {
            mdb.SetPC(address);
        } catch (DebugException ex) {
            throw new MException("Could not set pc. Debugger exception: " + ex.getMessage(), ex);
        }
    }
    /**
     * Use run and set breakpoint to skip over one line of code. You must be
     * halted and connected to issue this call.
     * Works only for SessionType DEBUGGER
     * @return pc after step
     * @throws MException
     */
    public long stepOver() throws MException {
        //System.out.println("Debugger::stepOver IN");
        validateAction(RequestedAction.STEP);
        long pc = -1;
        try {
            mdb.StepOver();
            waitFor(sessionState.HALTED);
            pc = getPC();
        } catch (DebugException ex) {
            throwException("Debugger::stepOver", ex);
        }
        //System.out.println("Debugger::stepOver OUT");
        return pc;
    }
    /**
     * Single step on instruction. You must be
     * halted and connected to issue this call.
     * Works only for SessionType DEBUGGER
     * @return pc after step
     * @throws MException
     */
    public long stepInstr() throws MException {
        validateAction(RequestedAction.STEP);
        long pc = -1;
        try {
            mdb.StepInstr();
            pc = getPC();
        } catch (DebugException ex) {
            throwException("Debugger::stepInstr", ex);
        }
        return pc;
    }
    /**
     * Step into any calls. You must be
     * halted and connected to issue this call.
     * Works only for SessionType DEBUGGER
     * @return pc after step
     * @throws MException
     */
    public long stepIn() throws MException {
        validateAction(RequestedAction.STEP);
        long pc = -1;
        try {
            mdb.StepIn();
            pc = getPC();
        } catch (DebugException ex) {
            throwException("Debugger::stepIn", ex);
        }
        return pc;
    }
    /**
     * Marking as private since I think the debugger has not implemented
     * this yet.
     * Works only for SessionType DEBUGGER
     * @return pc after step
     * @throws MException
     */
    private long stepOut() throws MException {
        validateAction(RequestedAction.STEP);
        long pc = -1;
        try {
            mdb.StepOut();
            pc = getPC();
        } catch (DebugException ex) {
            throwException("Debugger::stepOut", ex);
        }
        return pc;
    }
    /**
     * Read the entire contents of a device into the memory objects, then
     * save the memory objects into a hex file. Note that this call will
     * kill any debugging session. This is not a DEBUG READ operation.
     * Works for debugger/programmer.
     * @param fileName name of hex file to create
     * @throws MException
     */
    public void upload(String fileName) throws MException {
        validateAction(RequestedAction.UPLOAD);
        try {
            mdb.Upload();
            initLoader();
            loader.Save(fileName,false);
        } catch (LoadException ex) {
            throwException("Debugger:upload into file, LoadException while saving to file :" + fileName, ex);
        } catch (DebugException ex) {
            throwException("Debugger:upload into file", ex);
        }
    }
    /**
     * Read the entire contents of a device into the memory objects.
     * Note that this call will
     * kill any debugging session. This is not a DEBUG READ operation.
     * Works for debugger/programmer.
     * @throws MException
     */
    public void upload() throws MException {
        validateAction(RequestedAction.UPLOAD);
        try {
            mdb.Upload();
        } catch (DebugException ex) {
            throwException("Debugger:upload into memory", ex);
        }
    }
    
    /**
     * Load into scripting engine a script file module. Execution of The 
     * functions defined in this module will be available via 
     * executeScriptFunction. The runtime script context of the engine is
     * available via the getScriptObject and setScriptObject functions.
     * @param fileName name of file containing scripts
     * @throws com.microchip.mdbcs.MException
     */
    public void loadScriptFile(String fileName) throws MException {
        File fileToLoad = findScriptFile(fileName);
        try {
            scriptMediator.loadScript(ScriptRunner.Type.JYTHON2_7, fileToLoad);
        } catch (RuntimeException ex) {
            throwException("Debugger:loadScriptFile", ex);
        }
    }

    private File findScriptFile(String fileName) throws MException {
        File file = new File(fileName);
        File fileToLoad = file;
        if (file.isAbsolute()) {
            if (!file.exists()) {
                throw new MException(String.format("Debugger:loadScriptFile, file %s does not exist\n", fileName));
            }
        } else {
            if (file.exists()) {
                fileToLoad = file.getAbsoluteFile();
            } else {
                throw new MException(String.format("Debugger:loadScriptFile, file %s or %s does not exist\n", fileName, fileToLoad.getAbsoluteFile()));
            }
        }
        return fileToLoad;
    }
    
    /**
     * Execute functionName that is part of a module loaded via loadScriptFile
     * @param functionName function to be run
     * @param params any parameters being passed to the function
     * @return 
     */
    public Object executeScriptFunction(String functionName, Object ... params) throws MException {
        Object functionReturnValue = null;
        try {
            functionReturnValue = scriptRunner.executeFunction(functionName, params);
        } catch (RuntimeException ex) {
            throwException("Debugger:executeScriptFunction", ex);
        }
        return functionReturnValue;
    }
    
    /**
     * Given the name of an object, returns its content. This allows access
     * to global variables in the script loaded by loadScriptFile. This allows
     * you access to the engine runtime context.
     * @param objectName name of object whose value will be retrieved
     * @return object referenced by objectName
     * @throws MException 
     */
    public Object getScriptObject(String objectName) throws MException {
        Object objectValue= null;
        try {
            objectValue = scriptRunner.getObject(objectName);
        } catch (RuntimeException ex) {
            throwException("Debugger:getScriptObject", ex);
        }
        return objectValue;
    }
    
    /**
     * Add an object given a name and a value. This allows you to inject
     * globals into the script engine runtime context.
     * @param objectName
     * @param value
     * @throws MException 
     */
    public void setScriptObject(String objectName, Object value) throws MException {
        try {
            scriptRunner.put(objectName, value);
        } catch (RuntimeException ex) {
            throwException("Debugger:setScriptObject", ex);
        }
    }
    
    /**
     * Finish the debugging session. Debugger cannot be used unless
     * it gets re-constructed again.
     * Works for debugger/programmer.
     */
    public void disconnect() {
        mdb.Detach(this, tool);
        mdb.Disconnect();
    }
    /**
     * Helper function to idle
     * Works for debugger/programmer.
     * Can be called before connect
     * @param millis
     */
    public void sleep(long millis) {
        try {
            Thread.sleep(millis);
        } catch (InterruptedException ex) {
            Exceptions.printStackTrace(ex);
        }
    }

    /**
     * Returns the tools properties. Please look at MPLAB_X/doc/MDBCore.htm
     * for a description of tool properties and how to use them.
     * Can be called before connect
     * Works for debugger/programmer.
     * @return Properties
     */
    public Properties getToolProperties() {
        return toolProperties;
    }
    /**

    /**
     * Returns the tools assembly. Please look at MPLAB_X/doc/MDBCore.htm
     * for a description of the tool assembly and how to use it.
     * Can be called before connect
     * Works for debugger/programmer.
     * @return Assembly
     */
    public Assembly getToolAssembly() {
        return assembly;
    }
    /**


     * Tell ICD3 or PICkit3 to supply power to the target. If the property
     * "voltagevalue" is not set, the debugger will use the default value
     * as specified in the PIC file. This function should be called before connect.
     * The effect of this function is to alter the way connect works.
     * Works for debugger/programmer.
     * SHOULD be called before connect
     * @param state
     * @throws MException
     */
    public void supplyVDD(boolean state) throws MException {
        toolProperties = assembly.GetToolProperties();
        if (toolProperties == null) {
            throw new MException("Debugger::supplyVDD, could not get properties");
        }
        if (state)
            toolProperties.setProperty(valueForVddEnabled, "true");
        else
            toolProperties.setProperty(valueForVddEnabled, "false");
    }

    /**
     * Tell ICD3 or PICkit 3 to power target and to set voltage to the
     * passed value. This function should be called before connect.
     * The effect of this function is to alter the way connect works.
     * Works for debugger/programmer.
     * SHOULD be called before connect
     * @param vdd
     * @throws MException
     */
    public void setVDD(float vdd) throws MException {
        if (meta.getName().toLowerCase().contains("real")) {
            throw new MException("Debugger::setVDD, real ice cannot control power");
        }
        String vddAsString = Float.toString(vdd);
        toolProperties = assembly.GetToolProperties();
        if (toolProperties == null) {
            throw new MException("Debugger::setVDD, could not get properties");
        }
        toolProperties.setProperty(keyForVddProperty, vddAsString);
        toolProperties.setProperty(valueForVddEnabled, "true");
    }
    /**
     * Get access to the xPIC object. The xPIC object describes the device
     * You can get a lot of information about a device from the xPIC object
     * Works for debugger/programmer.
     * Can be called before connect
     * @return an instance of the XPIC for this session
     */
    public xPIC getPic() {
        return assembly.GetDevice();
    }

     /**
     * Tells if the device selected has EE Data.
     * Works for debugger/programmer.
     * @return Boolean
     */
    public Boolean hasEEData(){

        xPIC pdev = getPic();
        Boolean hasEEData;

        hasEEData = pdev.hasEEData();
        return hasEEData;
    }

    /**
     * Tells if the device selected has User ID.
     * Works for debugger/programmer.
     * @return Boolean
     */
    public Boolean hasUserID(){

        xPIC pdev = getPic();
        Boolean hasUserID;

        hasUserID = pdev.hasUserID();
        return hasUserID;

    }
     /**
     * Tells if the device selected has Boot Flash.
     * Works for debugger/programmer.
     * @return Boolean
     */
    
    public Boolean hasBootMemory(){

        xPIC pdev = getPic();
        Boolean hasBootMemory;

        hasBootMemory = pdev.hasBootConfig();
        return hasBootMemory;

    }
    /**
     * Some devices have a memory map that is dynamic. Given the value of
     * config bits, the program memory might look different. So, for these
     * devices, we assume that the config bits have been set (a call
     * to load or program) has been done. In that case we use the ProgramMemory
     * class to tell us how big it is.
     * @return
     * @throws MException 
     */
    Pair<Long, Long> getDynamicMemoryRange() throws MException {
        Pair<Long, Long> res = new Pair<Long, Long>(0L,0L);
        Class clazz = ProgramMemory.class;
        if (!noRefreshMemoriesMap.containsKey(clazz)) {
            throw new MException(String.format("Debugger:getDynamicMemoryRange, device does not contain memory %s", clazz));
        }
        MemoryModel memModel = noRefreshMemoriesMap.get(clazz);
        Collection<ComponentMemory> memories = memModel.GetComponentMemories();   
        
        
        Collection<ComponentMemory> mems = memModel.GetComponentMemories();
        Iterator<ComponentMemory> iterator = mems.iterator();
        ComponentMemory mem = iterator.next();
        res.first = mem.BaseAddress();
        xMemTraits memTraits = getPic().getMemTraits();
        long addrInc = memTraits.getCodeWordTraits().getAddrInc();
        //res.second = next.Size() / getPic().getMemTraits()
        long size = mem.Size();
        res.second = size /addrInc;
        //System.out.printf("(0x%x, 0x%x)\n", res.first, res.second);
        return res;
        
    }
    /**
     * Given the type of memory, return the address range for that memory
     * Works for debugger/programmer.
     * @param MemoryType
     * @return Pair<Long, Long> values for start and end addresses
     * @throws MException
     * @author Bhavik
     */
    public Pair<Long, Long> getMemoryRange(String memoryType) throws MException{

        xPIC pdev = getPic();
        Pair<Long, Long> res = null;

        // Memory type = ProgMem, EEData, DataMem, ConfigMem, UserID

        if(memoryType.equals("ProgMem")){

            res = pdev.getCodeRange();
            if (res.first == 0 && res.second == 0) {
                // we might be dealing with a part that has runtime defined
                // program memory. Like the dual partition parts where there
                // the program memory map is defined by the value of the config
                // bits.
                res = getDynamicMemoryRange();
            }
            return res;
        }
        if(memoryType.equals("EEData")){

            res = pdev.getEEDataRange();
            return res;
        }
        if(memoryType.equals("DataMem")){

            res = pdev.getDataSpaceRange();
            return res;
        }
        if(memoryType.equals("ConfigMem")){

            res = pdev.getDCRRange();
            return res;
        }
        if(memoryType.equals("UserID")){

            res = pdev.getUserIDRange();
            return res;
        }
        else{
            throw new MException("Debugger::getMemoryRange, could not getMemoryRange for" + memoryType);
        }

    }
    public Pair<String, Long> getFileAndLineAsLongFromAddress(long address) {
        Pair<String, Long> res = null;
        AddrToLn info = null;
        try {
            info = translator.addressToSourceLine(address);
            res = new Pair<String, Long>(info.file, info.lLine);
        } catch (TranslatorException ex) {
            // return null
        }
        return res;
    }

    /**
     * Given a program address, return the name of the file and the line number
     * where this code lives.
     * Works for debugger/programmer.
     * Can be called before connect
     * @param address
     * @return Pair<String, Sring> strings for File and Line
     */
    public Pair<String, String> getFileAndLineFromAddress(long address) {
        Pair<String, String> res = null;
        Pair<String, Long> p = getFileAndLineAsLongFromAddress(address);
        if (p != null) {
            String line = Long.toString(p.second);
            res = new Pair<String, String>(p.first, line);
        }
        return res;
    }

    /**
     * Given a file name and a line number return the address where the code
     * lives in program memory
     * Works for debugger/programmer.
     * Can be called before connect
     * @param fileName
     * @param line
     * @return an address
     */
    public long getAddressFromFileAndLine(String fileName, long line) {
        long res = -1;
        try {
            LnToAddr info = translator.sourceLineToAddress(fileName, line);
            res = info.lStartAddr & 0x00000000ffffffff;
        } catch (TranslatorException ex) {
            // return -1
        }
        return res;
    }

    /**
     * Given the name of a variable, return the address.
     * Works for debugger/programmer.
     * Can be called before connect
     * @param symbol
     * @return address for symbol
     */
    public long getSymbolAddress(String symbol) {
        Symbol info = (Symbol) symbolViewer.getRawSymbol(symbol);
        return info.Address() & 0x00000000ffffffffL;
    }
    /**
     * Given the name of a variable, return the address.
     * Works for debugger/programmer.
     * Can be called before connect
     * @param symbol
     * @return address for symbol
     */
    public long getSymbolSize(String symbol) {
        Symbol info = (Symbol) symbolViewer.getRawSymbol(symbol);
        return symbolViewer.getIntrinsicSizeInformation(info);
    }

    /**
     * Use this function to access SFR/NMMRs by name.
     * Works only for SessionType DEBUGGER. Should be halted and connected.
     * @param registerName
     * @return value of register read directly from the target
     * @throws MException
     */
    public long readRegister(String registerName) throws MException {
        long res = -1;
        try {
            SFRegister info = registerViewer.resolve(registerName);
            registerViewer.readSFRValue(info, true);
            res = info.getCurrentValue();
        } catch (RegisterViewException ex) {
            throwException("Debugger::readRegister, exception: ", ex);
        }
        return res;
    }
    /**
     * Use this function to access SFR/NMMRs by name.
     * Works only for SessionType DEBUGGER. Should be halted and connected.
     * @param registerName
     * @param value
     * @throws MException
     */
    public void writeRegister(String registerName, long value) throws MException {
        long res = -1;
        try {
            SFRegister info = registerViewer.resolve(registerName);
            info.setCurrentValue(value);
            registerViewer.writeSFRValue(info);
        } catch (RegisterViewException ex) {
            throwException("Debugger::writeRegister, exception: ", ex);
        }
    }

    /**
     * Read file registers. Debugger must be connected and halted.
     * Works only for SessionType DEBUGGER. 
     * @param address
     * @param size
     * @param data
     * @throws MException
     */
    public void readFileRegisters(long address, long size, byte [] data) throws MException {
        VirtualMemory mem = fileRegisters.GetVirtualMemory();
        if (!mem.IsValidAddress(address)) {
            // file register are represented in program memory for ARM devices
            allocateDynamicExternalMemory(address, size, fileRegisters);
            if (!mem.IsValidAddress(address)) {
                mem = programMemory.GetVirtualMemory();
                if (!mem.IsValidAddress(address)) {
                    allocateDynamicExternalMemory(address, size, programMemory);
                    
                    if (!mem.IsValidAddress(address)) {
                        throw new MException(String.format("Debugger:readFileRegisters address 0x%x is not a valid address", address));
                
                    }
                }
            }
        }
        mem.RefreshFromTarget(address, size);
        long readBytes = mem.Read(address, size, data);
        if ( readBytes != size)  {
            throw new MException(String.format("Debugger:readFileRegisters requested read of 0x%x bytes, got 0x%x", size, readBytes));
        }
    }
     /**
     * Read peripheral registers on PIC32. Debugger must be connected and halted.
     * Works only for SessionType DEBUGGER AND on PIC32 targets (use readFileRegisters for all other devices)
     * @param address
     * @param size
     * @param data
     * @throws MException
     */
    public void readPeripheralRegisters(long address, long size, byte[] data) throws MException {
        if (!pic.getFamily().equals(Family.PIC32)) {
            throw new MException("Debugger:readPeripheralRegisters on non PIC32 device");
        }
        VirtualMemory mem = peripheralRegisters.GetVirtualMemory();
        mem.RefreshFromTarget(address, size);
        long readBytes = mem.Read(address, size, data);
        if (readBytes != size) {
            throw new MException(String.format("Debugger:readPeripheralRegisters requested read of 0x%x bytes, got 0x%x", size, readBytes));
        }
    }
    /**
     * Write file registers. Debugger must be connected and halted.
     * Works only for SessionType DEBUGGER.
     * @param address
     * @param size
     * @param data
     * @throws MException
     */
    public void writeFileRegisters(long address, long size, byte [] data) throws MException {
        VirtualMemory mem = fileRegisters.GetVirtualMemory();
        if (!mem.IsValidAddress(address)) {
            // file register are represented in program memory for ARM devices
            mem = programMemory.GetVirtualMemory();
            if (!mem.IsValidAddress(address)) {
                throw new MException(String.format("Debugger:writeFileRegisters address 0x%x is not a valid address", address));
            }
        }
        long writtenBytes = mem.Write(address, size, data);
        if ( writtenBytes != size)  {
            throw new MException(String.format("Debugger:writeFileRegisters requested write of 0x%x bytes, wrote  0x%x", size, writtenBytes));
        }
        mem.CommitToTarget();
    }
    /**
     * <p>Return interface to access memory directly. You can use readMemory and
     * writeMemory instead.
     * Works for debugger/programmer.</p?>
     * 
     * <p>Can be called before connect. The parameter clazz, for example, should be one of:</p>
     * <ul>
     * <li>com.microchip.mplab.mdbcore.memory.memorytypes.ProgramMemory</li>
     * <li>com.microchip.mplab.mdbcore.memory.memorytypes.ConfigurationBits</li>
     * <li>com.microchip.mplab.mdbcore.memory.memorytypes.EEData</li>
     * <li>com.microchip.mplab.mdbcore.memory.memorytypes.BootMemory</li>
     * <li>com.microchip.mplab.mdbcore.memory.memorytypes.UserID</li>
     * <li>com.microchip.mplab.mdbcore.memory.memorytypes.FlashData</li>
     * <li>com.microchip.mplab.mdbcore.memory.memorytypes.DataFlash</li>
     * </ul>
     * <p>The memories above can be read/written when SessionType is PROGRAMMER.
     * The data sheet will tell you which memories exist in your device.
     * </p>
     * <ul>
     * <li>com.microchip.mplab.mdbcore.memory.memorytypes.PeripheralMemory</li>
     * <li>com.microchip.mplab.mdbcore.memory.memorytypes.FileRegisters</li>
     * </ul>
     * <p>The memories above can be read/written when SessionType is DEBUGGER.
     * The data sheet will tell you which memories exist in your device.
     * </p>     
     * @param clazz
     * @return An instance of the MemoryInterface to the memory object
     * @throws MException
     */
    public Memory getMemory(Class clazz) throws MException {
        if (!noRefreshMemoriesMap.containsKey(clazz)) {
            throw new MException(String.format("Debugger:getMemory, device does not contain memory %s", clazz));

        }
        return noRefreshMemoriesMap.get(clazz).GetVirtualMemory();
        
    }
    /**
     * Access the memory objects buffers in MDBCore that represent the memories in the device.
     * Works for debugger/programmer.
     * Can be called before connect.
     * Note that FileRegisters, NMMR and PeriphealRegisters cannot be accessed
     * via this call. Use readFileRegister/writeFileRegisters, readRegister/writeRegister
     * 
     * @param clazz 
     * @param address
     * @param size
     * @param data
     * @throws MException
     * 
     * @see #getMemory(java.lang.Class) list of memories you can pass in clazz parameter
     */
    public void readMemory(Class clazz, long address, long size, byte [] data) throws MException {
        if (!noRefreshMemoriesMap.containsKey(clazz)) {
            throw new MException(String.format("Debugger:readMemory, device does not contain memory %s", clazz));

        }
        VirtualMemory mem = noRefreshMemoriesMap.get(clazz).GetVirtualMemory();
        long readBytes = mem.Read(address, size, data);
        if ( readBytes != size)  {
            throw new MException(String.format("Debugger:readMemory requested read of 0x%x bytes, got 0x%x", size, readBytes));
        }
        
    }
    /**
     * Access the memory objects buffers.
     * Works for debugger/programmer.
     * Can be called before connect
     * Note that FileRegisters, NMMR and PeriphealRegisters cannot be accessed
     * via this call. Use readFileRegister/writeFileRegisters, readRegister/writeRegister
     * @param clazz
     * @param address
     * @param size
     * @param data
     * @throws MException
     * 
     * @see #getMemory(java.lang.Class) list of memories you can pass in clazz parameter
     */
    public void writeMemory(Class clazz, long address, long size, byte [] data) throws MException {
        if (!noRefreshMemoriesMap.containsKey(clazz)) {
            throw new MException(String.format("Debugger:writeMemory, device does not contain memory %s", clazz));

        }
        VirtualMemory mem = noRefreshMemoriesMap.get(clazz).GetVirtualMemory();
        long wroteBytes = mem.Write(address, size, data);
        ((Subject)mem).Notify(new MemoryEvent(MemoryEvent.EVENTS.MEMORY_CHANGED));
        if ( wroteBytes != size)  {
            throw new MException(String.format("Debugger:writeMemory requested write of 0x%x bytes, wrote 0x%x", size, wroteBytes));
        }
    }

    /**
     * Get a list of all possible memories
     * @param clazz
     * @return 
     * 
     * @see #getMemory(java.lang.Class) list of memories you can pass in clazz parameter
     */
    public Collection GetComponentMemories (Class clazz){
        MemoryModel memModel = noRefreshMemoriesMap.get(clazz);
        Collection<ComponentMemory> memories = memModel.GetComponentMemories();
        return memories;
    }


     /**
     * How big a memory object is
     * Works for debugger/programmer.
     * Can be called before connect.
     * @param clazz
     * @return number of bytes in memory object. Note: 24bit opcodes are stored
     * as 32 bit words with one unused byte.
     * @throws MException
     * 
     * @see #getMemory(java.lang.Class) list of memories you can pass in clazz parameter
     */
     public long getMemorySize(Class clazz) throws MException {
        long res = -1;
        if (!noRefreshMemoriesMap.containsKey(clazz)) {
            throw new MException(String.format("Debugger:getMemorySize, device does not contain memory %s", clazz));

        }
        MemoryModel memModel = noRefreshMemoriesMap.get(clazz);
        Collection<ComponentMemory> memories = memModel.GetComponentMemories();
        res = memModel.GetComponentMemories().iterator().next().Size();
        return res;
    }
    /**
     * Base address of the memory object.
     * Works for debugger/programmer.
     * Can be called before connect.
     * Use the base value as the lowest address to be passed ro
     * readMemory and writeMemory
     * @param clazz
     * @return value of lowest location in memory object
     * @throws MException
     */
    public long getMemoryBase(Class clazz) throws MException {
        long res = -1;
        if (!noRefreshMemoriesMap.containsKey(clazz)) {
            throw new MException(String.format("Debugger:getMemorySize, device does not contain memory %s", clazz));

        }
        MemoryModel memModel = noRefreshMemoriesMap.get(clazz);
        Collection<ComponentMemory> memories = memModel.GetComponentMemories();
        res = memModel.GetComponentMemories().iterator().next().BaseAddress();
        return res;
    }

    /**
     * How many breakpoints are left in this debug session
     * Works only for SessionType DEBUGGER.
     * Target must be connected and halted.
     * @return number of left over breakpoints
     * @throws MException
     */
    public int getNumAvailableBP() throws MException {
        WritableControlPointStore wpcs = getWCPS();
        int n = wpcs.getNumberAvailableProgramControlPoints();
        controlPointMediator.releaseWritableControlPointStore(wpcs);
        return n;
    }
    /**
     * Max number of breakpoints supported by this device
     * Works only for SessionType DEBUGGER
     * Can be called before connect
     * @return max num of breakpoints for this device
     */
    public int getNumMaxBP() {
        return controlPointMediator.getEmulationCapabilities().getMaxInstructionBPs();
    }
    /**
     * Returns a positive number representing which breakpoint was set. The
     * number is 0->Max-1 where Max is the value returned by getNumMaxBP.
     * If not breakpoints are available it returns -1
     * You can get how many breakpoints are available by calling
     * getFirstAvailableWCP
     * Works only for SessionType DEBUGGER
     * Target must be connected and halted.
     * 
     * @param address
     * @return breakpoint number
     * @throws MException
     */
    public int setBP(long address) throws MException {
        int res = -1;
        if (getNumAvailableBP() > 0) {
            WritableControlPointStore wcps = getWCPS();
            WritableControlPoint addressBP = wcps.getNewControlPoint();
            validateObject(addressBP, "Debugger::BP, could not create new breakpoint");
            addressBP.setBreakType(BreakType.PROGRAM);
            addressBP.setBreakAddress(address);
            addressBP.setEnabled(true);
            controlPointMediator.commitAndReleaseWritableControlPointStore(wcps);
            // We need to remember addressBP. We put it into a list.
            // See if we can reuse an entry in the list
            if (breakpoints.size() > 0) {
                int index = 0;
                for(WritableControlPoint w : breakpoints) {
                    if (w == null) {
                        breakpoints.set(index, addressBP);
                        res = index;
                        break;
                    }
                    index++;
                }
            }
            if (res == -1) {
                // No empty slots. Just add it to the end of the list
                breakpoints.add(addressBP);
                res = breakpoints.size() - 1;
            }
        } else {
            throw new MException("Debugger::setBP, all breakpoints have been used");
        }
        return res;
    }

    public int setSWBP(long address) throws MException {
        int res = -1;
        WritableControlPointStore wcps = getWCPS();
        WritableControlPoint addressBP = wcps.getNewControlPoint();
        validateObject(addressBP, "Debugger::SWBP, could not create software new breakpoint");
        addressBP.setBreakType(BreakType.PROGRAM);
        addressBP.setBreakAddress(address);
        addressBP.setIsSoftwareBreakpoint(true);
        addressBP.setEnabled(true);
        controlPointMediator.commitAndReleaseWritableControlPointStore(wcps);
        // We need to remember addressBP. We put it into a list.
        // See if we can reuse an entry in the list
        if (breakpoints.size() > 0) {
            int index = 0;
            for(WritableControlPoint w : breakpoints) {
                if (w == null) {
                    breakpoints.set(index, addressBP);
                    res = index;
                    break;
                }
                index++;
            }
        }
        if (res == -1) {
            // No empty slots. Just add it to the end of the list
            breakpoints.add(addressBP);
            res = breakpoints.size() - 1;
        }

        return res;
    }
    
    /**
     * Returns a positive number representing which breakpoint was set. The
     * number is 0->Max-1 where Max is the value returned by getNumMaxBP.
     * If not breakpoints are available it returns -1
     * You can get how many breakpoints are available by calling
     * getFirstAvailableWCP
     * Works only for SessionType DEBUGGER
     * Target must be connected and halted.
     *
     * @param symbol
     * @return breakpoint number
     * @throws MException
     */
    public int setBP(String symbol) throws MException {
        long address = getSymbolAddress(symbol);
        return setBP(address);
    }
    /**
     * Pass the int returned by setBP to clear the breakpoint
     * Works only for SessionType DEBUGGER
     * Target must be connected and halted.
     * @param breakpointNumber
     * @throws MException
     */
    public void clearBP(int breakpointNumber) throws MException {
        if (breakpointNumber < breakpoints.size()) {
            WritableControlPointStore wcps = getWCPS();
            wcps.deleteControlPoint(breakpoints.get(breakpointNumber));
            controlPointMediator.commitAndReleaseWritableControlPointStore(wcps);
            breakpoints.set(breakpointNumber, null);
        } else {
            throw new MException(String.format("breakpoint number greater than max breakpoints (%d)", breakpoints.size()));
        }
    }
    /**
     * Returns true if target is halted.
     * Works only for SessionType DEBUGGER
     * @return true if halted
     */
    public boolean isHalted() {
        return sessionState.equals(SessionState.HALTED);
    }
    /**
     * Returns true if target is running.
     * Works only for SessionType DEBUGGER
     * @return true if running
     */
    public boolean isRunning() {
        return sessionState.equals(SessionState.RUNNING);
    }
    
    /**
     * Call this method to release all objects held by the Debugger.
     * After calling this method the Debugger object cannot longer be used.
     */
    public void destroy() {
        if (mdb != null) {
            try {
                mdb.Abort();
            } catch (DebugException ex) {
                Exceptions.printStackTrace(ex);
            }
        }
        if (assembly != null) {
            AssemblyFactory af = Lookup.getDefault().lookup(AssemblyFactory.class);
            if (af != null) {
               af.Destroy(assembly);
            }
        }
    
        
    }
    
    private static String cleanUpToolName(String toolName) {
        return toolName.replace(" ","").toLowerCase();
   }

    
    public static String getSerialNumber(PlatformTool pt) {
        return PlatformMPLABCommStringParser.getSerialNumber(pt.getToolDescriptor());
    }

    
    public static List<String> getSupportedToolNames() {
        List<String> res = new ArrayList<String>();
        List<PlatformToolMeta> allTools = PlatformToolMetaManager.getAllTools();
        for (PlatformToolMeta ptm : allTools) {
            res.add(ptm.getName());
        }
        return res;
    }
    
    public static List<PlatformTool> getListOfTools(String toolName) {
        PlatformToolBrowser ptf = new PlatformToolBrowser(null);
        ptf.scan();
        List<PlatformTool> existingTools = ptf.getExistingTools();
        List<PlatformTool> res = new ArrayList<PlatformTool>();
        String name = cleanUpToolName(toolName);
        for (PlatformTool pt: existingTools) {
                String cleanToolName = cleanUpToolName(pt.getName());
                if (cleanToolName.contains(name)) {
                    res.add(pt);
                }
            
        }
        ptf.bye();
        
        return res;
    }
    
    public static List<PlatformTool> getListOfToolIDs(String toolName) {
        PlatformToolBrowser ptf = new PlatformToolBrowser(null);
        ptf.scan();
        List<PlatformTool> existingTools = ptf.getExistingTools();
        List<PlatformTool> res = new ArrayList<PlatformTool>();
        for (PlatformTool pt: existingTools) {
                String cleanToolName = cleanUpToolName(pt.getName());
                if (toolName.equals(pt.getID())) {
                    res.add(pt);
                }
            
        }
        ptf.bye();
        
        return res;
    }
    /**
     * Given a tool name and an optional serialNumber, try to find the
     * PlatformTool object that represents that using the PlatformToolBrowser
     * @param toolName
     * @param serialNumber
     * @return
     */
    public static Pair<PlatformTool, PlatformToolMeta> findTool(String toolName, String serialNumber) throws MException {
        Pair<PlatformTool, PlatformToolMeta> res = null;
    
        if (toolName.toLowerCase().equals("sim") || toolName.toLowerCase().equals("simulator")) {
            PlatformToolMeta m = PlatformToolMetaManager.getTool("Simulator");
            return new Pair<PlatformTool, PlatformToolMeta>(null, m);
        }

        PlatformToolBrowser ptf = new PlatformToolBrowser(null);
        ptf.scan();
        List<PlatformTool> existingTools = ptf.getExistingTools();
        
        res = findToolNameByToolID(toolName, serialNumber, existingTools);
        if (res == null) {
            res = findToolNameByToolName(toolName, serialNumber, existingTools);
        }
        ptf.bye();
        return res;
    }

    // Private functions
    /**
     * Look in connected tools for a tool that matches the toolID (and serialNumber)
     * @param toolID
     * @param serialNumber
     * @param existingTools
     * @return 
     */
    private static Pair<PlatformTool, PlatformToolMeta> findToolNameByToolID(String toolID, String serialNumber, List<PlatformTool> existingTools) {
        Pair<PlatformTool, PlatformToolMeta> res = null;
        for (PlatformTool pt : existingTools) {
            String id = pt.getID();
            if (id.equals(toolID)) {
                if (serialNumber != null) {
                    String sn = getSerialNumber(pt);
                    if (sn == null) {
                        continue;
                    }
                    if (!serialNumber.equals(sn)) {
                        continue;
                    }
                }
                res = new Pair<PlatformTool, PlatformToolMeta>(pt, PlatformToolMetaManager.getTool(toolID));
                break;
            }
        }
        return res;
    }
    /**
     * Backwards compatible: try to guess toolID from toolName.
     * @param toolName
     * @param serialNumber
     * @param existingTools
     * @return 
     */
    private static Pair<PlatformTool, PlatformToolMeta> findToolNameByToolName(String toolName, String serialNumber, List<PlatformTool> existingTools) {
        Pair<PlatformTool, PlatformToolMeta> res = null;
        String name = cleanUpToolName(toolName);
        for (PlatformTool pt : existingTools) {
            String cleanToolName = cleanUpToolName(pt.getName());
            if (cleanToolName.contains(name)) {
                if (serialNumber != null) {
                    String sn = getSerialNumber(pt);
                    if (sn == null) {
                        continue;
                    }
                    if (!serialNumber.equals(sn)) {
                        continue;
                    }
                }
                res = new Pair<PlatformTool, PlatformToolMeta>(pt, PlatformToolMetaManager.getTool(pt.getID()));
                break;
            }
        }
        return res;
    }

    private void makeDebugger(String device, String toolName, boolean asDebugger, String serialNumber) throws MException {
        // need to set this property as soon as possible for it to take effect.
        getAnyProperties();
        System.setProperty("crownking.stream.verbosity", "quiet");
        
        this.device = device;
        this.serialNumber = serialNumber;
        
        Pair<PlatformTool, PlatformToolMeta> p = findTool(toolName, serialNumber);
        if (p == null) {
            String err = "No tools of type " + toolName + " where found";
            if (serialNumber != null) {
                err = "No tools of type " + toolName + " serial number " + serialNumber + " was found";
            }
            throw new MException(err);
        }
        tool = p.first;
        meta = p.second;
        
        if (asDebugger) {
            this.sessionType = SessionType.DEBUGGER;
        } else {
            this.sessionType = SessionType.PROGRAMMER;
        }
        init();
    }
    private Properties createDefaultProperties() {
        Properties props = new Properties();
        return props;
    }
    
    private void validateObject(Object obj, String errorMessage) throws MException {
        if (obj == null) {
            throw new MException(errorMessage, new NullPointerException());
        }
    }
    private void validateAction(RequestedAction action) throws MException {
        if ( (sessionState.compareTo(SessionState.DISCONNECTED) == 0)  &&
            !(action.equals(RequestedAction.CONNECT) || action.equals(RequestedAction.LOAD)) ) {
            throw new MException("Debugger::validateAction, need to connect first before doing a " +  action.toString());
        }
        if (sessionType.equals(sessionType.PROGRAMMER))  {
            if (action.equals(RequestedAction.HALT) ||
                action.equals(RequestedAction.STEP) ||
                action.equals(RequestedAction.RUN)  ||
                action.equals(RequestedAction.RESET) ) {
                throw new MException("Debugger::validateAction, cannot " + action.toString() + " in a programmer session\n");
            }
        }
        if (sessionType.equals(sessionType.DEBUGGER))  {
            if (action.equals(RequestedAction.SET_MCRL)) {
                throw new MException("Debugger::validateAction, cannot " + action.toString() + " in a debugger session\n");
            }
        }
        // Let's see if we are busy
        String exceptionString = null;
        if (lastToolEvent != null) {
            switch (sessionState) {
                case BUSY:
                    exceptionString = "Wait for current operation to complete";
                    break;
                case CONNECTED:
                    break;
                case DISCONNECTED:
                    exceptionString = "Need to connect first";
                    break;
                case HALTED:
                    break;
                case RUNNING:
                    break;
            }
        }
        if (exceptionString != null) {
            throw new MException(exceptionString);
        }
    }
    private void init() throws MException {
        noRefreshMemoriesList = initMemoryClasses();
        isSim = false;
        if (meta.getID().toLowerCase().equals("simulator")) {
            isSim = true;
        }
        numSCLAttachments = 0;
        if (assembly == null) {
            AssemblyFactory af = Lookup.getDefault().lookup(AssemblyFactory.class);
            validateObject(af, "Debugger::init, could not lookup AssemblyFactory class");
            if (header != null && !header.isEmpty())
                assembly = af.Create(device, header);
            else
                assembly = af.Create(device);
            validateObject(assembly, "Debugger::init could not create assembly");
            if (pluginboard != null && !pluginboard.isEmpty())
                assembly.SetPluginBoard(pluginboard);
            af.ChangeTool(assembly, meta.getID(), meta.getClassName(), meta.getFlavor(), tool == null ? null : tool.getToolDescriptor());
            toolProperties = createDefaultProperties();
            af.SetToolProperties(assembly, toolProperties);
            pic = assembly.GetDevice();
            initMemories();
        }
        if (translator == null) {
            translator = assembly.getLookup().lookup(ITranslator.class);
            validateObject(translator, "Debugger::init, could not lookup ITranslator");
        }
        if (symbolViewer == null) {
            symbolViewer = assembly.getLookup().lookup(SymbolViewProvider.class);
            validateObject(symbolViewer, "Debugger::init, could not lookup SymbolViewProvider");
        }
        if (registerViewer == null) {
            registerViewer = assembly.getLookup().lookup(RegisterViewProvider.class);
            validateObject(registerViewer, "Debugger::init, could not lookup RegisterViewProvider");
            registerViewer.getInitializedRegisters(true);
        }
        if (scriptMediator == null) {
            scriptMediator = assembly.getLookup().lookup(ScriptMediator.class);
            validateObject(scriptMediator, "Debugger::init, could not lookup ScriptMediator");
            scriptRunner = scriptMediator.getScriptRunner(ScriptRunner.Type.JYTHON2_7);
            validateObject(scriptRunner, "Debugger::init, could not get ScriptRunner");
        }
        if (isSim) {
             sim = assembly.lookup(Simulator.class);
             validateObject(sim, "Debugger::init, could not lookup Simulator");
        } else {
            sim = null;
        }
        // Does not work with PM3.
        if (!meta.getName().toLowerCase().contains("pm3")){
            if (controlPointMediator == null) {
                controlPointMediator  = assembly.getLookup().lookup(ControlPointMediator.class);
                validateObject(controlPointMediator, "Debugger::init, could not lookup ControlPointMediator");
            }
        }
        if (mdb == null) {
            mdb = (com.microchip.mplab.mdbcore.debugger.Debugger) assembly.getLookup().lookup(com.microchip.mplab.mdbcore.debugger.Debugger.class);
            validateObject(mdb, "Debugger::init could not lookup Debugger");
            // Attach so our Update method gets called with ToolEvents
            mdb.Attach(this, null);
        }
        setDebugger2(mdb);
        
        this.sessionState = SessionState.DISCONNECTED;
        setRespondYesToAllMessages(false);
    }
    
    private void setDebugger2(com.microchip.mplab.mdbcore.debugger.Debugger mdb) throws MException {
        if (Debugger2.class.isAssignableFrom(mdb.getClass())) {
            mdb2 = (Debugger2)mdb;
        } else {
            throw new MException("Internal fatal error. Could not get access to Debugger2 interface");
        }
    }
    
    private void throwException(String extraMessage, Exception ex) throws MException {
        Throwable cause = ex.getCause();
        if (cause instanceof MDBCommonToolException) {
            eCommonToolExceptionErrors error = ((MDBCommonToolException) cause).getInternalErrValue();
            throw new MException(extraMessage + " (" + error.name() + ", " + error.toString() + ")", ex);

        } else {
            throw new MException(extraMessage + " :" + ex.getMessage(), ex);
        }
    }
    private void initLoader() throws MException {
        if (loader == null) {
            loader = assembly.getLookup().lookup(Loader.class);
            validateObject(loader, "Debugger::loadFile could not lookup Loader");
        }
    }
    // Observer interface implementation
    public /* synchronized */ void Update(Object o) {
        {
            if (o instanceof ToolEvent) {
                lastToolEvent = ((ToolEvent) o).GetEvent();
                
                //System.out.println("Debugger::Update " + lastToolEvent.toString());
                switch (lastToolEvent) {
                    case RUN:
                        sessionState = SessionState.RUNNING;
                        break;
                    case ANIMATE_STEP:
                        sessionState = SessionState.BUSY;
                        break;
                    case HALT:
                        sessionState = SessionState.HALTED;
                        break;
                    case RESET:
                        sessionState = sessionState.RESET;
                        break;
                    case DISCONNECT:
                        sessionState = SessionState.DISCONNECTED;
                        break;
                    case KILL:
                        sessionState = SessionState.DISCONNECTED;
                        break;
                    case PROGRAM_START:
                        sessionState = SessionState.BUSY;
                        break;
                    case PROGRAM_DONE:
                        sessionState = sessionType.equals(SessionType.DEBUGGER) ? SessionState.HALTED : SessionState.CONNECTED;
                        break;
                    case UPLOAD_START:
                        sessionState = SessionState.BUSY;
                        break;
                    case UPLOAD_DONE:
                        sessionState = SessionState.CONNECTED;
                        break;
                    case CONNECTED:
                        sessionState = SessionState.CONNECTED;
                        break;
                    case DISCONNECTED:
                        sessionState = SessionState.DISCONNECTED;
                        break;
                }
            }
        }
    }
    /*
     private  void waitFor(SessionState st) {
        while (!this.sessionState.equals(st)) {
            sleep(100);
        }
    }
    */

     private void waitFor(SessionState st) throws MException {
        // try for a max of 5 seconds
         waitFor(st, DEFAULT_TIME_OUT);
    }

     private void waitFor(SessionState st, int timeOutMilliSecs) throws MException {
        // try for a max of 5 seconds
        long start = System.currentTimeMillis();
        long end   = start + timeOutMilliSecs;
        while (!this.sessionState.equals(st)) {
            if (System.currentTimeMillis() > end) {
                throw new MException(String.format("Debugger::waitFor, timeout %s operation\n", st));
            }
            Helper.sleep(100);
        }
    }

    private WritableControlPointStore getWCPS() throws MException {
        WritableControlPointStore wpcs = controlPointMediator.getWritableControlPointStore();
        validateObject(wpcs, "Debugger::getWPCS, unable to get a writable control point store");
        return wpcs;
    }
    private void initMemories() throws MException {

        programMemory = assembly.getLookup().lookup(ProgramMemory.class);
        // We have 2 kinds of noRefreshMemories:noRefreshMemories that need to be accessed at run time
        // (file registers and NMMRs) and so require a refresh from/to target.
        // And noRefreshMemories that are accessed without a target refresh.
        fileRegisters = assembly.getLookup().lookup(FileRegisters.class);
        if (pic.getFamily().equals(Family.PIC32)) {
            // for PIC32 these are two different types of memory
            peripheralRegisters = assembly.getLookup().lookup(PeripheralMemory.class);
        }
        validateObject(fileRegisters, "Debugger::init could not lookup file registers");
        // NMMR we handle via the ReadRegister and WriteRegister functions.
        // The rest:
        noRefreshMemoriesMap = new HashMap<Class, MemoryModel>();
        for(Class<?extends MemoryModel> c : noRefreshMemoriesList) {
            MemoryModel m = (MemoryModel) assembly.getLookup().lookup(c);
            if (m != null) {
                noRefreshMemoriesMap.put(c, m);
            }
        }
    }
    
    /**
     * isConnected may be used in cases the HW tool might go away for some
     * reason.
     * @return true if HW tool is connected 
     */
    public boolean isConnected() {
        return mdb.IsConnected();
    }
    /**
     * Look for locations.properties and read any properties from there and
     * put them in the global System properties.
     */
    private void getAnyProperties() throws MException {
        // see if packslib.packsfolder is already defined by the user
        String packsfolder = System.getProperty("packslib.packsfolder", "");
        String thirdpartyfolder = System.getProperty("mplabx.thirdparty.lib.path", "");
        if (packsfolder.isEmpty() || thirdpartyfolder.isEmpty()) {
               
            InputStream stream = Debugger.class.getResourceAsStream("location.properties");
            try {
                System.getProperties().load(stream);
            } catch (Exception ex) {
                // Internal version of mdbcs does not have a location.properties
                // The user is expected to set packslib.packsfolder on their own
            }
            packsfolder = System.getProperty("packslib.packsfolder", "");
            thirdpartyfolder = System.getProperty("mplabx.thirdparty.lib.path", "");
            if (packsfolder.isEmpty()) {
                System.err.println("PACKS location not defined. Will try to derive location from MPLAB X installation)");
                System.err.println("Use system property packslib.packsfolder to point to the packs directory in an MPLAB X installation.");
            } else {
                System.out.println("PACKS location (using packs from MPLAB X installation mdbcs was built against) = " + packsfolder);
            }
            if (thirdpartyfolder.isEmpty()) {
                System.err.println("Thirdparty lib location not defined. Will try to derive location from MPLAB X installation)");
                System.err.println("Use system property mplabx.thirdparty.lib.path to point to the Thirdparty lib directory in an MPLAB X installation.");
            } else {
                System.out.println("Thirdparty lib location (using packs from MPLAB X installation mdbcs was built against) = " + thirdpartyfolder);
            }
        } else {
            System.out.println("PACKS location (defined by packslib.packsfolder property) = " + packsfolder);
            System.out.println("Thirdparty lib location (defined by packslib.packsfolder property) = " + thirdpartyfolder);
        }
    }

    /**
     * If address and size are not valid for virtual memory interface of provided 
     * MemoryModel, an attempt is made to dynamically allocate as external memory
     * if the address and size can be located in a external region supported by the 
     * device. (copied from SymbolViewDefault.java)
     * 
     * @param address long
     * @param size long 
     * @param mem MempryModel
     * @return 
     */
    private boolean allocateDynamicExternalMemory(long address, long size, MemoryModel mem) {
        boolean result = false;
        
        if (pic.getMainPartition().hasExternalInstRegions(false) && !mem.GetVirtualMemory().IsValidAddress(address)) {
            if (mem.GetComponentMemories().stream().anyMatch((memComponent) -> (memComponent.IsExternal()))
                    && mem.CanAdjustModelToFitExternalRange(address, size)) {

                //Looks like address islocated in external memory that has not been 
                //dynamically allocated. Add memory range to component.
                result = mem.AdjustModelToFitExternalRange(address, size, true);
            }
        }
        
        return result;
    }
}

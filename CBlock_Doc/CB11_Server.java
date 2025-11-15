//------------------------------------------------------------------------------
// CB11_Server.java -- Example server code for C-Block Basics #11.
//------------------------------------------------------------------------------
// Full project code and documentation is available here:
//    https://github.com/robdunn4/QSpice
//
// Note: The below instructions work for Java 21 (LTS) and later.  For earlier
//       versions, your mileage may vary.
//
// To manually start the server program without compiling:
//   java CB11_Server.java <port number>
//
// To compile Java (creates Server.class):
//   javac CB11_Server.java
//
// To manually start the server from class file (CB11_Server.class):
//   java CB11_Server <port number>
//
// To create JAR file:
//   (1) Compile as above to create CB11_Server.class
//   (2) Create a manifest file (e.g., manifest.txt) with the line:
//         Main-Class: CB11_Server
//   (3) Create the JAR file with the command:
//         jar cvfm CB11_Server.jar manifest.txt CB11_Server.class
//
// To manually start server from JAR file:
//   java -jar CB11_Server.jar <port number>
//
// To change the QSPICE component to use the JAR file, change the component
// "server=" attribute from *.java to *.jar.  For example:
//   From --> server=localhost:1024/CB11_Server.java
//   To   --> server=localhost:1024/CB11_Server.jar
//

import java.io.*;
import java.net.*;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;

public class CB11_Server {
  // == Per-Instance Data Goes Here ==
  // note: it's not necessary to declare the per-instance data in a class
  // but done here for clarity...
  private class InstData {
    int portNbr = 0; // server port number (for user messages & logging)
    int gainVal = 0; // input Gain attribute (from client)
    int stepNbr = 0; // current simulation step number (from client)
    String logName; // logfile path (from client)
    String instName; // component instance name (from client)
    PrintWriter logFile; // log printer
    boolean postProcess = false; // end simulation flag (from client event)
  }

  InstData instData = new InstData();

  // === Command Constants ===
  // default template message IDs
  private static final int PORT_EVALUATE = 2;
  private static final int PORT_MAXSTEP = 3;
  private static final int PORT_TRUNCATE = 4;
  // custom message IDs
  private static final int PORT_LOGNAME = 1024; // set logfile name string
  private static final int PORT_INSTNAME = 1025; // set instance name string
  private static final int PORT_GAIN = 1026; // set gain int val
  private static final int PORT_STEPNBR = 1027; // set step # int val
  private static final int PORT_DESTROY = 1028; // event: client Destroy()
  private static final int PORT_POSTPROCESS = 1029; // event: client PostProcess()
  private static final int PORT_INITIALZE = 1030; // cmd: initialize server with current values
  private static final int PORT_CLOSESERVER = 1031; // cmd: close server

  // === Stream Buffers at Class Scope ===
  private DataInputStream in;
  private DataOutputStream out;

  // Client socket at class scope to close on errors
  private Socket clientSocket;

  // Server I/O buffer at class scope for convenience
  private static final int BUFFER_SIZE = 1024; // must match client DLL
  // create an underlying raw data buffer for messages
  private static final byte[] buffer = new byte[BUFFER_SIZE];
  // wrap the buffer in a ByteBuffer to enable reading/writing ints, doubles,
  // etc., with little-endian translation
  private static final ByteBuffer byteBuffer = ByteBuffer.wrap(buffer)
      .order(ByteOrder.LITTLE_ENDIAN);

  public static void main(String[] args) {
    int port = 0;

    // get and validate port parameter
    if (args.length != 1) {
      System.err.println("Usage: java Server <port number>");
      System.exit(-1);
    }
    try {
      port = Integer.parseInt(args[0]);
    } catch (NumberFormatException e) {
      System.err.println("Invalid port number: " + args[0]);
      System.exit(-1);
    }

    // launch server process
    CB11_Server server = new CB11_Server();
    int retVal = server.run(port);
    System.exit(retVal);
  }

  // the server process/main loop
  // TODO: clean up the exception handling
  public int run(int port) {
    // save port number in per-instance data
    instData.portNbr = port;

    try (ServerSocket serverSocket = new ServerSocket(port)) {
      System.out.println("Server listening on port " + port);
      clientSocket = serverSocket.accept();
      try (
          DataInputStream din = new DataInputStream(clientSocket.getInputStream());
          DataOutputStream dout = new DataOutputStream(clientSocket.getOutputStream());) {
        in = din;
        out = dout;

        // main loop
        int cnt;
        while ((cnt = in.read(buffer, 0, BUFFER_SIZE)) > 0) {
          int command = byteBuffer.getInt(0);
          switch (command) {
            case PORT_EVALUATE:
              handlePortEvaluate(); // handler sends return msg
              break;

            case PORT_MAXSTEP:
              byteBuffer.putDouble(0, handlePortMaxStep());
              out.write(buffer, 0, 8);
              break;

            case PORT_TRUNCATE:
              byteBuffer.putDouble(0, handlePortTruncate());
              out.write(buffer, 0, 8);
              break;

            case PORT_LOGNAME:
              instData.logName = new String(buffer, 4, cnt - 4 - 1,
                  StandardCharsets.UTF_8);
              out.write(buffer, 0, 4);
              break;

            case PORT_INSTNAME:
              instData.instName = new String(buffer, 4, cnt - 4 - 1,
                  StandardCharsets.UTF_8);
              out.write(buffer, 0, 4);
              break;

            case PORT_GAIN:
              instData.gainVal = byteBuffer.getInt(4);
              out.write(buffer, 0, 4);
              break;

            case PORT_STEPNBR:
              instData.stepNbr = byteBuffer.getInt(4);
              out.write(buffer, 0, 4);
              break;

            case PORT_DESTROY:
              handlePortDestroy();
              out.write(buffer, 0, 4);
              break;

            case PORT_POSTPROCESS:
              handlePortPostProcess();
              out.write(buffer, 0, 4);
              break;

            case PORT_INITIALZE:
              handlePortInitialize();
              out.write(buffer, 0, 4);
              break;

            case PORT_CLOSESERVER:
              handlePortCloseServer();
              closeClientSocket();
              // do not send response msg
              return 0;

            default: // Invalid command code; exit ungracefully
              System.err.println("Invalid command code: " + command);
              closeClientSocket();
              return -1;
          }
        }
      } catch (SocketException e) {
        // Handle client disconnection gracefully
        System.err.println("Socket exception: " + e.getMessage());
        closeClientSocket();
        return -1;
      } catch (IOException e) {
        // Handle other I/O errors; exit ungracefully
        System.err.println("Error handling client: " + e.getMessage());
        closeClientSocket();
        return -1;
      }
    } catch (IOException e) {
      // Handle server socket errors; exit ungracefully
      System.err.println("Could not listen on port " + port);
      return -1;
    }

    // Close client socket if still open; exit normally
    closeClientSocket();
    return 0;
  }

  // === Handler Methods ===
  // note: the evaluation function handler must be modified to match the client
  // DLL code. that is, the number of inputs and outputs depends on
  // the component DLL code. because both inputs and outputs may change,
  // this code must be hand-modified to match the client DLL.
  private void handlePortEvaluate() throws IOException {
    // must match client!
    double t = byteBuffer.getDouble(4);
    double IN = byteBuffer.getDouble(12);
    double EN = byteBuffer.getDouble(20);

    // Implememnt evaluation here (example)
    double OUT = 0;
    if (EN != 0)
      OUT = IN * instData.gainVal;

    // must match client!!!
    byteBuffer.putDouble(0, OUT);
    out.write(buffer, 0, 8);

    // write logfile entry
    instData.logFile.println("Evaluation function IN: t=" + t + ", IN=" + IN +
        ", EN=" + EN + " || OUT: OUT=" + OUT);
  }

  private double handlePortMaxStep() {
    double t = byteBuffer.getDouble(4);
    double retVal = 1e308;

    // set retVal to a number other than 1e308 to stipulate a max timestep.
    // you can use the current timepoint ("t") and instance data to determine
    // an appropriate maximum timestep.

    // write logfile entry
    instData.logFile.println("MaxStep() function:     t=" + t);

    return retVal;
  }

  private double handlePortTruncate() throws IOException {
    // must match client!
    double t = byteBuffer.getDouble(4);
    double IN = byteBuffer.getDouble(12);
    double EN = byteBuffer.getDouble(20);
    double timestep = byteBuffer.getDouble(28);
    double retVal = 1e308;

    // set retVal to a number other than 1e308 to tactically stipulate a shorter
    // timestep. you can use the above inputs and the per-instance data to
    // determine an appropriate timestep. however, do not modify the
    // per-instance data here without restoring it. (or create Trunc()-specific
    // per-instance data elements.)

    // write logfile entry
    instData.logFile.println("Truncate function:      t=" + t + ", IN=" + IN +
        ", EN=" + EN + ", timestep=" + timestep + " || Returning " + retVal);

    return retVal;
  }

  private void handlePortDestroy() {
    // do final processing for simulation step for component instance before
    // releasing resources, closing logs, etc.
    instData.logFile.println("Instance " + instData.instName +
        ": handlePortDestroy() -- end simulation step");

    if (instData.postProcess) {
      // do final processing for simulation for component instance before
      // releasing resources, closing logs, etc.
      instData.logFile.println("Instance " + instData.instName +
          ": handlePortDestroy() -- end of simulation");
    }

    // release resources, close logfile, etc.
    instData.logFile.close();
  }

  private void handlePortPostProcess() {
    instData.logFile.println("Instance " + instData.instName +
        ": handlePortPostProcess()");
    instData.postProcess = true;
  }

  private void handlePortInitialize() {
    // open/allocate resources that depend on initialized per-instance data,
    // e.g., logfile
    try {
      if (instData.stepNbr > 1)
        instData.logFile = new PrintWriter(new FileWriter(instData.logName, true));
      else
        instData.logFile = new PrintWriter(new FileWriter(instData.logName));
    } catch (Exception e) {
      System.out.println("Error opening logfile: " + instData.logName + " (" + e.getMessage() + ")");
      System.exit(-1); // ungracefully terminate
    }

    instData.logFile.println("Initializing:  Instance=" + instData.instName +
        " Port=" + instData.portNbr + " LogName=" + instData.logName +
        " StepNbr=" + instData.stepNbr + " GainVal=" + instData.gainVal);
  }

  private void handlePortCloseServer() {
    // last chance to do something before closing socket and exiting
    // note: logFile already closed under normal circumstances...
  }

  private void closeClientSocket() {
    System.out.println("Closing client socket on port " + instData.portNbr + "...");
    if (clientSocket != null && !clientSocket.isClosed()) {
      try {
        clientSocket.close();
        System.out.println("Client socket closed.");
      } catch (IOException e) {
        System.err.println("Failed to close client socket: " + e.getMessage());
      }
    }
  }
}
//------------------------------------------------------------------------------
// CB12_HostServer.java -- Example server code for C-Block Basics #12.
//------------------------------------------------------------------------------
// Full project code and documentation is available here:
//    https://github.com/robdunn4/QSpice
//
// Note: The below instructions work for Java 21 (LTS) and later.  For earlier
//       versions, your mileage may vary.
//
// To manually start the server program without compiling:
//   java CB12_HostServer.java <port number>
//
// To compile Java (creates CB12_HostServer.class):
//   javac CB12_HostServer.java
//
// To manually start the server from class file (CB12_HostServer.class):
//   java CB12_HostServer <port number>
//
// To create JAR file:
//   (1) Compile as above to create CB12_HostServer.class
//   (2) Create a manifest file (e.g., manifest.txt) with the line:
//         Main-Class: CB12_HostServer
//   (3) Create the JAR file with the command:
//         jar cvfm CB12_HostServer.jar manifest.txt CB12_HostServer.class
//
// To manually start server from JAR file:
//   java -jar CB12_HostServer.jar <port number>
//

import java.io.*;
import java.net.*;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class CB12_HostServer {
  private static final int BUFFER_SIZE = 1024; // must match client DLL

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
  private static final int PORT_GETPORTNBR = 1032; // get: get server port number

  private final int listenPort;
  private volatile boolean running = true;
  private ServerSocket serverSocket;
  private final ExecutorService workerPool = Executors.newCachedThreadPool();

  public static void main(String[] args) {
    if (args.length != 1) {
      System.err.println("Usage: java CB12_HostServer <port number>");
      System.exit(-1);
    }
    int port = 0;
    try {
      port = Integer.parseInt(args[0]);
    } catch (NumberFormatException e) {
      System.err.println("Invalid port number: " + args[0]);
      System.exit(-1);
    }

    CB12_HostServer server = new CB12_HostServer(port);
    server.start();
  }

  public CB12_HostServer(int port) {
    this.listenPort = port;
  }

  public void start() {
    try (ServerSocket ss = new ServerSocket(listenPort)) {
      this.serverSocket = ss;
      System.out.println("Host server listening on port " + listenPort);
      System.out.println("Ctrl+C to close server");

      // Install shutdown hook to close the ServerSocket when JVM exits
      Runtime.getRuntime().addShutdownHook(new Thread(() -> {
        System.out.println("Ctrl+C detected.  Shutting down server...");
        running = false;
        try {
          if (serverSocket != null && !serverSocket.isClosed())
            serverSocket.close();
        } catch (IOException ignored) {
        }
        workerPool.shutdownNow();
      }));

      while (running) {
        try {
          Socket client = serverSocket.accept();
          System.out.println("Accepted connection from " +
              client.getRemoteSocketAddress());
          workerPool.submit(new ClientHandler(client));
        } catch (SocketException se) {
          // This can happen on shutdown when serverSocket.close() is called.
          if (running) {
            System.err.println("Socket exception accepting client: " +
                se.getMessage());
          }
        }
      }
    } catch (IOException e) {
      System.err.println("Could not listen on port " + listenPort + ": " +
          e.getMessage());
    } finally {
      shutdownAndAwaitTermination();
    }
  }

  private void shutdownAndAwaitTermination() {
    running = false;
    try {
      if (serverSocket != null && !serverSocket.isClosed())
        serverSocket.close();
    } catch (IOException ignored) {
    }
    workerPool.shutdownNow();
    System.out.println("Host server stopped.");
  }

  // === Per-client handler ===
  private class ClientHandler implements Runnable {
    // Per-instance data -- need not be a class but made one for clarity
    private class InstData {
      int portNbr = 0;
      int gainVal = 0;
      int stepNbr = 0;
      String logName = "";
      String instName = "";
      PrintWriter logFile = null;
      boolean postProcess = false;
    }

    private final InstData instData = new InstData();

    private final Socket socket;
    private final byte[] buffer = new byte[BUFFER_SIZE];
    private final ByteBuffer byteBuffer = ByteBuffer.wrap(buffer)
        .order(ByteOrder.LITTLE_ENDIAN);

    ClientHandler(Socket s) {
      this.socket = s;
      instData.portNbr = s.getPort();
    }

    @Override
    public void run() {
      try (DataInputStream in = new DataInputStream(socket.getInputStream());
          DataOutputStream out = new DataOutputStream(socket.getOutputStream())) {

        int cnt;
        while ((cnt = in.read(buffer, 0, BUFFER_SIZE)) > 0) {
          // read command code (first 4 bytes)
          int command = byteBuffer.getInt(0);
          try {
            switch (command) {
              case PORT_EVALUATE:
                handlePortEvaluate(out);
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
                instData.logName = new String(buffer, 4, cnt - 4,
                    StandardCharsets.UTF_8).trim();
                out.write(buffer, 0, 4);
                break;

              case PORT_INSTNAME:
                instData.instName = new String(buffer, 4, cnt - 4,
                    StandardCharsets.UTF_8).trim();
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
                closeClient();
                // do not send response msg
                return;

              case PORT_GETPORTNBR:
                byteBuffer.putInt(0, instData.portNbr);
                out.write(buffer, 0, 4);
                break;

              default:
                System.err.println("Invalid command code from " +
                    socket.getRemoteSocketAddress() + ": " + command);
                closeClient();
                return;
            }
          } catch (Exception ex) {
            // Per-client errors should close the client but not the server.
            System.err.println("Error handling client " +
                socket.getRemoteSocketAddress() + ": " + ex.getMessage());
            closeClient();
            return;
          }
        }

        // client disconnected normally
        closeClient();

      } catch (SocketException se) {
        System.err.println("Socket exception for client " +
            socket.getRemoteSocketAddress() + ": " + se.getMessage());
        closeClient();
      } catch (IOException e) {
        System.err.println("I/O error for client " +
            socket.getRemoteSocketAddress() + ": " + e.getMessage());
        closeClient();
      }
    }

    // === Handler Methods ===
    // note: the evaluation function handler must be modified to match the client
    // DLL code. that is, the number of inputs and outputs depends on
    // the component DLL code. because both inputs and outputs may change,
    // this code must be hand-modified to match the client DLL.
    private void handlePortEvaluate(DataOutputStream out) throws IOException {
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
      instData.logFile.println("Evaluation function IN: t=" + t +
          ", IN=" + IN + ", EN=" + EN + " || OUT: OUT=" + OUT);
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
      instData.logFile.println("Truncate function:      t=" + t +
          ", IN=" + IN + ", EN=" + EN + ", timestep=" + timestep +
          " || Returning " + retVal);
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

        instData.logFile.close();
        instData.logFile = null;
      }
    }

    private void handlePortPostProcess() {
      instData.logFile.println("Instance " + instData.instName +
          ": handlePortPostProcess()");
      instData.postProcess = true;
    }

    private void handlePortInitialize() throws Exception {
      // open/allocate resources that depend on initialized per-instance data,
      // e.g., logfile
      if (instData.stepNbr > 1)
        instData.logFile = new PrintWriter(new FileWriter(instData.logName, true));
      else
        instData.logFile = new PrintWriter(new FileWriter(instData.logName));

      instData.logFile.println("Initializing:  Instance=" + instData.instName +
          " Port=" + instData.portNbr + " LogName=" + instData.logName +
          " StepNbr=" + instData.stepNbr + " GainVal=" + instData.gainVal);
    }

    private void handlePortCloseServer() {
      // last chance to do something before closing this client connection
      // note: logfile is already closed
      System.out.println("Instance " + instData.instName +
          ": handlePortCloseServer() -- client connection closing");
    }

    private void closeClient() {
      try {
        if (socket != null && !socket.isClosed()) {
          socket.close();
          System.out.println("Closed client connection: " +
              socket.getRemoteSocketAddress());
        }
      } catch (IOException e) {
        System.err.println("Failed to close client socket: " + e.getMessage());
      }
    }
  }
}

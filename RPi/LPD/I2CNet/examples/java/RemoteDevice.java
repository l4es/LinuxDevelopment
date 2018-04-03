package net.sf.lucistria.xmas.i2c;

import static java.lang.String.format;

import java.io.IOException;
import java.net.Socket;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.List;
import java.util.logging.Logger;

import org.apache.commons.lang3.ArrayUtils;

public class RemoteDevice {
	class WatchDog implements Runnable {
		private RemoteDevice remoteDevice;
		private long stopTime;
		private long timeout;
		
		private boolean stopped = false;

		public WatchDog(RemoteDevice remoteDevice, long timeout) {
			this.remoteDevice = remoteDevice;
			this.timeout = timeout;
			this.tick();
			
			new Thread(this).start();
		}
		
		public synchronized void tick() {
			this.stopTime = new Date().getTime() + timeout;
		}
		
		public synchronized void stop() {
			stopped = true;
			this.notifyAll();
		}
		
		public synchronized void run() {
			long currentTime = new Date().getTime();
			while (stopTime > currentTime && !stopped) {
				try {
					this.wait(stopTime - currentTime);
				} catch (InterruptedException e) {
					remoteDevice.close();
					Logger.getAnonymousLogger().severe(e.toString());
					e.printStackTrace();
				}
				currentTime = new Date().getTime();
			}
			if (!stopped) {
				Logger.getAnonymousLogger().severe("Watchdog expired.  Closing remote device.");
				remoteDevice.close();
			}
		}
	}
	
	private static final int READ_OPERATION = 0;
	private static final int WRITE_OPERATION = 1;
	
	public static final List<Byte> START_FRAME = Arrays.asList((byte) 'a');
	public static final List<Byte> END_FRAME = Arrays.asList((byte) 'b');
	public static final List<Byte> SPECIAL = Arrays.asList((byte) 'z');
	public static final List<Byte> SPECIAL_START_FRAME = Arrays.asList((byte) 'z', (byte) 'w');
	public static final List<Byte> SPECIAL_END_FRAME = Arrays.asList((byte) 'z', (byte) 'x');
	public static final List<Byte> SPECIAL_SPECIAL = Arrays.asList((byte) 'z', (byte) 'y');

	private Socket socket;
	private int busIndex;
	private int device;
	private String host;
	private int port;
	
	public RemoteDevice(String host, int port, int busIndex, int device) {
		this.host = host;
		this.port = port;
		this.busIndex = busIndex;
		this.device = device;
		try {
			socket = new Socket(host, port);
		} catch (IOException e) {
			throw new RuntimeException(e);
		}
	}

	public void write(int location, List<Byte> data) {
		this.write(location, WRITE_OPERATION, new ArrayList<Byte>(data));
	}
	
	public List<Byte> read(int location, int length) {
		return this.read(location, length, new ArrayList<>());
	}
	
	public List<Byte> read(int location, int length, List<Byte> data) {
		data = new ArrayList<>(data);
		data.add(0, (byte) length);
		this.write(location, READ_OPERATION, data);
		
		WatchDog watchDog = new WatchDog(this, 30000);
		try {
			List<Byte> packet = new ArrayList<>();
			while (true) {
				byte[] inputBuffer = new byte[1024];
				int inputLength = socket.getInputStream().read(inputBuffer);
				watchDog.tick();
				if (inputLength < 0) {
					throw new IllegalStateException("Socket has been closed by remote host.");
				}
				if (inputLength == 0) {
					return new ArrayList<>();
				}
				
				byte[] dataBuffer = new byte[inputLength];
				System.arraycopy(inputBuffer, 0, dataBuffer, 0, inputLength);
				
				List<Byte> receivedData = new ArrayList<>(Arrays.asList(ArrayUtils.toObject(dataBuffer)));
				packet.addAll(receivedData);
				
				if (packet.get(0) != 'a') {
					packet.clear();
				}
				if (packet.size() > 0 && packet.get(packet.size() - 1) == 'b') {
					break;
				}
			}
			watchDog.stop();
			packet.remove(0);
			packet.remove(packet.size() - 1);
			this.substitute(SPECIAL_START_FRAME, START_FRAME, packet);
			this.substitute(SPECIAL_END_FRAME, END_FRAME, packet);
			this.substitute(SPECIAL_SPECIAL, SPECIAL, packet);
			
			return packet;
		} catch (IOException e) {
			throw new RuntimeException(e);
		}
	}
	
	public void close() {
		try {
			socket.close();
		} catch (IOException e) {
			throw new RuntimeException(e);
		}
	}
	
	public String toString() {
		return format("Host: %s, Port: %s, Bus: %s, Device 0x%02X", host, port, busIndex, device);
	}
	
	private void write(int location, int operation, List<Byte> data) {
		try {
			data.addAll(0, Arrays.asList((byte) busIndex, (byte) device, (byte) location, (byte) operation));
			this.substitute(SPECIAL, SPECIAL_SPECIAL, data);
			this.substitute(START_FRAME, SPECIAL_START_FRAME, data);
			this.substitute(END_FRAME, SPECIAL_END_FRAME, data);
			data.add(0, (byte) 'a');
			data.add((byte) 'b');
			
			socket.getOutputStream().write(ArrayUtils.toPrimitive(data.toArray(new Byte[0])));
		} catch (IOException e) {
			throw new RuntimeException(e);
		}
	}
	
	private void substitute(List<Byte> target, List<Byte> substitute, List<Byte> packet) {
		for (int i = 0; i < packet.size() - target.size() + 1; ++i) {
			if (target.equals(packet.subList(i, i + target.size()))) {
				for (int j = 0; j < target.size(); ++j) {
					packet.remove(i);
				}
				packet.addAll(i, substitute);
			}
		}
	}
}

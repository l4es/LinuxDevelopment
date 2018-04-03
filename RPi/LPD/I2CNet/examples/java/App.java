import java.util.Arrays;
import java.util.List;

import net.sf.lucistria.xmas.i2c.RemoteDevice;

public class App {
	public static void main(String[] args) {
		RemoteDevice remoteDevice = new RemoteDevice("10.10.10.10", 7337, 1, 0x52);

		// Setup some device configuration, the following performs a write followed by a read.
		List<Byte> result = remoteDevice.read(0x00, 1, Arrays.asList((byte) 0x21));
		if (result.size() != 1) {
			throw new RuntimeException("Failed to read from device: " + remoteDevice.toString());
		}
		if (result.get(0) != 0x21) {
			throw new RuntimeException("Failed to set write mode: " + remoteDevice.toString());
		}

		// Now we write some data.  In this case, we do not care about the read.  This is not likely to block.
		remoteDevice.write(0x06, Arrays.asList((byte) 0x01, (byte) 0x00, (byte) 0x01, (byte) 0x00));

		// Read some more data.  All reads will block.
		result = remoteDevice.read(0x01, 2);
		System.out.println(String.format("Current temperature: %d", ((int) result.get(1)) << 8 | result.get(0)));

		// This will perform a blocking write, essentially it is a zero length read.
		remoteDevice.read(0x00, 0, Arrays.asList((byte) 0x21));

		// Now we are done, close the device to clean up the socket.
		remoteDevice.close();
	}
}

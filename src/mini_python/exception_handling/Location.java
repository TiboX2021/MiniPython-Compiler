package mini_python.exception_handling;

/**
 * Code location in the source file
 */
public class Location {
	final int line;
	final int column;

	public Location(int line, int column) {
		this.line = line + 1;
		this.column = column;
	}

	@Override
	public String toString() {
		return this.line + ":" + this.column + ":";
	}
}
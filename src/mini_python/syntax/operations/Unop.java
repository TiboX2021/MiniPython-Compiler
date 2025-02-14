package mini_python.syntax.operations;

import mini_python.typing.Type;

public enum Unop {
    Uneg("negation"), Unot("not");

    public final String opName;

    Unop(String opName) {
        this.opName = opName;
    }

    /**
     * Given a type, build the resulting type of the operation.
     * Returns null if the operation is invalid
     */
    public Type coerce(Type type) {
        if (type == Type.DYNAMIC) {
            return Type.DYNAMIC;
        }

        switch (this) {
            case Uneg:
                if (type == Type.INT64 || type == Type.BOOL) {
                    return Type.INT64;
                }
                break;
            case Unot:
                return Type.BOOL; // All types can be coerced to bool
        }

        return null; // Default output: coercion failed
    }
}

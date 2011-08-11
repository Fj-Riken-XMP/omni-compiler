/*
 * $TSUKUBA_Release: $
 * $TSUKUBA_Copyright:
 *  $
 */

package exc.xcalablemp;

import exc.block.*;
import exc.object.*;
import java.util.*;

public class XMPutil {
  private static final String LOOP_ITER = "XCALABLEMP_LOOP_ITER_PROP";

  public static void putLoopIter(CforBlock b, String indVarName, XobjList loopIter) {
    HashMap<String, XobjList> loopIterTable = (HashMap<String, XobjList>)b.getProp(LOOP_ITER);
    if (loopIterTable == null) {
      loopIterTable = new HashMap<String, XobjList>();
      b.setProp(LOOP_ITER, (Object)loopIterTable);
    }

    loopIterTable.put(indVarName, loopIter);
  }

  public static XobjList getLoopIter(CforBlock b, String indVarName) {
    HashMap<String, XobjList> loopIterTable = (HashMap<String, XobjList>)b.getProp(LOOP_ITER);
    if (loopIterTable == null) {
      return null;
    } else {
      return loopIterTable.get(indVarName);
    }
  }

  // FIXME array can be a dynamic size
  public static long getArrayElmtCount(Xtype type) {
    if (type.isArray()) {
      ArrayType arrayType = (ArrayType)type;
      long arraySize = arrayType.getArraySize();
      return arraySize * getArrayElmtCount(arrayType.getRef());
    }
    else return 1;
  }

  public static XMPpair<Ident, Xtype> findTypedVar(String name, PragmaBlock pb) throws XMPexception {
    Ident id = pb.findVarIdent(name);
    if (id == null)
      throw new XMPexception("cannot find '" + name + "'");

    Xtype type = id.Type();
    if (type == null)
      throw new XMPexception("'" + name + "' has no type");

    return new XMPpair<Ident, Xtype>(id, type);
  }

  public static boolean hasCommXMPpragma(BlockList bl) {
    BlockIterator i = new bottomupBlockIterator(bl);
    for(i.init(); !i.end(); i.next()) {
      Block b = i.getBlock();
      if (b.Opcode() == Xcode.XMP_PRAGMA) {
        PragmaBlock pb = (PragmaBlock)b;
        String pragmaName = pb.getPragma();

        switch (XMPpragma.valueOf(pragmaName)) {
          // FIXME case REFLECT: needed???
          case BARRIER:
          case REDUCTION:
          case BCAST:
          // FIXME case GMOVE: needed???
            return true;
          default:
            break;
        }
      }
    }

    return false;
  }

  public static boolean isIntegerType(Xtype type) {
    if (type.getKind() == Xtype.BASIC) {
      switch (type.getBasicType()) {
        case BasicType.CHAR:
        case BasicType.UNSIGNED_CHAR:
        case BasicType.SHORT:
        case BasicType.UNSIGNED_SHORT:
        case BasicType.INT:
        case BasicType.UNSIGNED_INT:
        case BasicType.LONG:
        case BasicType.UNSIGNED_LONG:
        case BasicType.LONGLONG:
        case BasicType.UNSIGNED_LONGLONG:
          return true;
        default:
          return false;
      }
    }
    else return false;
  }

  public static boolean isZeroIntegerObj(Xobject x) {
    switch (x.Opcode()) {
      case INT_CONSTANT:
        if (x.getInt() == 0) {
          return true;
        }
        else {
          return false;
        }
      default:
        return false;
    }
  }

  public static String getTypeName(Xtype type) {
    if (type.getKind() == Xtype.BASIC) {
      switch (type.getBasicType()) {
        case BasicType.BOOL:			return new String("BOOL");
        case BasicType.CHAR:			return new String("CHAR");
        case BasicType.UNSIGNED_CHAR:		return new String("UNSIGNED_CHAR");
        case BasicType.SHORT:			return new String("SHORT");
        case BasicType.UNSIGNED_SHORT:		return new String("UNSIGNED_SHORT");
        case BasicType.INT:			return new String("INT");
        case BasicType.UNSIGNED_INT:		return new String("UNSIGNED_INT");
        case BasicType.LONG:			return new String("LONG");
        case BasicType.UNSIGNED_LONG:		return new String("UNSIGNED_LONG");
        case BasicType.LONGLONG:		return new String("LONGLONG");
        case BasicType.UNSIGNED_LONGLONG:	return new String("UNSIGNED_LONGLONG");
        case BasicType.FLOAT:			return new String("FLOAT");
        case BasicType.DOUBLE:			return new String("DOUBLE");
        case BasicType.LONG_DOUBLE:		return new String("LONG_DOUBLE");
        case BasicType.FLOAT_IMAGINARY:		return new String("FLOAT_IMAGINARY");
        case BasicType.DOUBLE_IMAGINARY:	return new String("DOUBLE_IMAGINARY");
        case BasicType.LONG_DOUBLE_IMAGINARY:	return new String("LONG_DOUBLE_IMAGINARY");
        case BasicType.FLOAT_COMPLEX:		return new String("FLOAT_COMPLEX");
        case BasicType.DOUBLE_COMPLEX:		return new String("DOUBLE_COMPLEX");
        case BasicType.LONG_DOUBLE_COMPLEX:	return new String("LONG_DOUBLE_COMPLEX");
        default:				return null;
      }
    }
    else {
      return null;
    }
  }

  public static int countElmts(XobjList list) {
    int count = 0;
    Iterator<Xobject> it = list.iterator();
    while (it.hasNext()) {
      it.next();
      count++;
    }

    return count;
  }

  public static int countElmts(XobjList list, int constant) {
    int count = 0;
    Iterator<Xobject> it = list.iterator();
    while (it.hasNext()) {
      Xobject x = it.next();
      if (x == null) continue;

      if (x.Opcode() == Xcode.INT_CONSTANT) {
        if (x.getInt() == constant)
          count++;
      }
    }

    return count;
  }

  public static int countElmts(XobjList list, String string) {
    int count = 0;
    Iterator<Xobject> it = list.iterator();
    while (it.hasNext()) {
      Xobject x = it.next();
      if (x == null) continue;

      if (x.Opcode() == Xcode.STRING) {
        if (x.getString().equals(string))
          count++;
      }
    }

    return count;
  }

  public static boolean hasElmt(XobjList list, int constant) {
    Iterator<Xobject> it = list.iterator();
    while (it.hasNext()) {
      Xobject x = it.next();
      if (x == null) continue;

      if (x.Opcode() == Xcode.INT_CONSTANT) {
        if (x.getInt() == constant)
          return true;
      }
    }

    return false;
  }

  public static boolean hasElmt(XobjList list, String string) {
    Iterator<Xobject> it = list.iterator();
    while (it.hasNext()) {
      Xobject x = it.next();
      if (x == null) continue;

      if (x.Opcode() == Xcode.STRING) {
        if (x.getString().equals(string))
          return true;
      }
    }

    return false;
  }

  public static boolean hasIdent(XobjList list, String string) {
    Iterator<Xobject> it = list.iterator();
    while (it.hasNext()) {
      Ident id = (Ident)it.next();
      if (id == null) {
        continue;
      }
      
      if (id.getName().equals(string)) {
        return true;
      }
    }
    
    return false;
  }

  public static int getFirstIndex(XobjList list, int constant) throws XMPexception {
    int index = 0;
    Iterator<Xobject> it = list.iterator();
    while (it.hasNext()) {
      Xobject x = it.next();
      if (x != null) {
        if (x.Opcode() == Xcode.INT_CONSTANT) {
          if (x.getInt() == constant)
            return index;
        }
      }

      index++;
    }

    throw new XMPexception("exception in exc.xcalablemp.XMPutil.getFirstIndex(), element does not exist");
  }

  public static int getFirstIndex(XobjList list, String string) throws XMPexception {
    int index = 0;
    Iterator<Xobject> it = list.iterator();
    while (it.hasNext()) {
      Xobject x = it.next();
      if (x != null) {
        if (x.Opcode() == Xcode.STRING) {
          if (x.getString().equals(string))
            return index;
        }
      }

      index++;
    }

    throw new XMPexception("exception in exc.xcalablemp.XMPutil.getFirstIndex(), element does not exist");
  }

  public static int getLastIndex(XobjList list, int constant) throws XMPexception {
    int elmtIndex = 0;
    boolean hasFound = false;

    int index = 0;
    Iterator<Xobject> it = list.iterator();
    while (it.hasNext()) {
      Xobject x = it.next();
      if (x != null) {
        if (x.Opcode() == Xcode.INT_CONSTANT) {
          if (x.getInt() == constant) {
            hasFound = true;
            elmtIndex = index;
          }
        }
      }

      index++;
    }

    if (hasFound) return elmtIndex;
    else
      throw new XMPexception("exception in exc.xcalablemp.XMPutil.getLastIndex(), element does not exist");
  }

  public static int getLastIndex(XobjList list, String string) throws XMPexception {
    int elmtIndex = 0;
    boolean hasFound = false;

    int index = 0;
    Iterator<Xobject> it = list.iterator();
    while (it.hasNext()) {
      Xobject x = it.next();
      if (x != null) {
        if (x.Opcode() == Xcode.STRING) {
          if (x.getString().equals(string)) {
            hasFound = true;
            elmtIndex = index;
          }
        }
      }

      index++;
    }

    if (hasFound) return elmtIndex;
    else
      throw new XMPexception("exception in exc.xcalablemp.XMPutil.getLastIndex(), element does not exist");
  }
}

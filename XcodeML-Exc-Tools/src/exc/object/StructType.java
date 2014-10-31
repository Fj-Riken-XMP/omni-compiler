/* 
 * $TSUKUBA_Release: Omni OpenMP Compiler 3 $
 * $TSUKUBA_Copyright:
 *  PLEASE DESCRIBE LICENSE AGREEMENT HERE
 *  $
 */
package exc.object;

/**
 * Represents C-struct / Fortran-type type.
 */
public class StructType extends CompositeType
{
    public StructType(String id, XobjList id_list, int typeQualFlags, Xobject gccAttrs,
                      Xobject[] codimensions)
    {
        super(Xtype.STRUCT, id, id_list, typeQualFlags, gccAttrs, codimensions);
    }

    public StructType(String id, XobjList id_list, int typeQualFlags, Xobject gccAttrs)
    {
        this(id, id_list, typeQualFlags, gccAttrs, null);
    }

    @Override
    public Xobject getFnumElementsExpr()
    {
        return Xcons.IntConstant(1);
    }

    @Override
    public Xobject getFelementLengthExpr()
    {
        throw new UnsupportedOperationException
          ("Restriction: could not get size of a structure");
    }

    @Override
    public int getFelementLength()
    {
        throw new UnsupportedOperationException
          ("Restriction: could not get size of a structure as integer");
    }


    @Override
    public Xtype copy(String id)
    {
        StructType t = new StructType(id, getMemberList(), getTypeQualFlags(),
                                      getGccAttributes(), copyCodimensions());
        t.original = (original != null) ? original : this;
        t.tag = (tag != null) ? tag : ((original != null) ? original.tag : null);
        return t;
    }
}

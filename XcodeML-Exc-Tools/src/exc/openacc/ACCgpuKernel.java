package exc.openacc;

import java.util.*;
import exc.block.*;
import exc.object.*;

public class ACCgpuKernel {
  private final ACCinfo kernelInfo; //parallel or kernels info
  private final ACCgpuManager gpuManager;
  private static final String ACC_GPU_FUNC_PREFIX = "_ACC_GPU_FUNC";
  private static final String ACC_REDUCTION_VAR_PREFIX = "_ACC_reduction_";
  private static final String ACC_CACHE_VAR_PREFIX = "_ACC_cache_";
  private static final String ACC_REDUCTION_TMP_VAR = "_ACC_GPU_RED_TMP";
  private static final String ACC_REDUCTION_CNT_VAR = "_ACC_GPU_RED_CNT";
  static final String ACC_GPU_DEVICE_FUNC_SUFFIX = "_DEVICE";
  private final Xobject _accThreadIndex = Xcons.Symbol(Xcode.VAR, Xtype.intType, "_ACC_thread_x_id");
  private final Xobject _accBlockIndex = Xcons.Symbol(Xcode.VAR, Xtype.intType, "_ACC_block_x_id");
  private final Xobject _accSyncThreads = ACCutil.getMacroFuncId("_ACC_GPU_M_BARRIER_THREADS", Xtype.voidType).Call();
  private final Xobject _accAsyncSync = Xcons.Symbol(Xcode.VAR, Xtype.intType, "ACC_ASYNC_SYNC");

  private final List<Block> _kernelBlocks;
  private List<Ident> _outerIdList;
  private Set<Ident> _readOnlyOuterIdSet;
  private final ArrayDeque<Loop> loopStack = new ArrayDeque<Loop>();
  private final SharedMemory sharedMemory = new SharedMemory();
  private final ReductionManager reductionManager = new ReductionManager();
  private final Set<Ident> _useMemPoolOuterIdSet = new HashSet<Ident>();
  private final List<XobjList> allocList = new ArrayList<XobjList>(); //for private array or reduction tmp array
  private List<ACCvar> _outerVarList;

  public ACCgpuKernel(ACCinfo kernelInfo, List<Block> kernelBlocks) {
    this.kernelInfo = kernelInfo;
    this._kernelBlocks = kernelBlocks;
    this.gpuManager = new ACCgpuManager(kernelInfo);
  }

  private void collectOuterVar() {
    _outerVarList = new ArrayList<ACCvar>();
    for(Ident id : _outerIdList){
      ACCvar accvar = kernelInfo.getACCvar(id);
      if(accvar == null) accvar = kernelInfo.findOuterACCvar(id.getName());

      if(accvar == null){
        ACC.fatal(id.getName() + " not found");
      }else{
        _outerVarList.add(accvar);
      }
    }
  }

  //ok!
  public Block makeLaunchFuncCallBlock() {
    List<Block> kernelBody = _kernelBlocks;
    String funcName = kernelInfo.getFuncInfo().getArg(0).getString();
    int lineNo = kernelBody.get(0).getLineNo().lineNo();
    String launchFuncName = ACC_GPU_FUNC_PREFIX + "_" + funcName + "_L" + lineNo;

    collectOuterVar();

    //make deviceKernelDef
    String deviceKernelName = launchFuncName + ACC_GPU_DEVICE_FUNC_SUFFIX;
    XobjectDef deviceKernelDef = makeDeviceKernelDef(deviceKernelName, _outerIdList, kernelBody);

    //make launchFuncDef
    XobjectDef launchFuncDef = makeLaunchFuncDef(launchFuncName, deviceKernelDef);

    //add deviceKernel and launchFunction
    XobjectFile devEnv = kernelInfo.getGlobalDecl().getEnvDevice();
    devEnv.add(deviceKernelDef);
    devEnv.add(launchFuncDef);

    //make launchFuncCall
    FunctionType launchFuncType = (FunctionType) launchFuncDef.getFuncType();
    launchFuncType.setFuncParamIdList(launchFuncDef.getDef().getArg(1));
    Ident launchFuncId = kernelInfo.getGlobalDecl().declExternIdent(launchFuncDef.getName(), launchFuncType);
    XobjList launchFuncArgs = makeLaunchFuncArgs();

    return Bcons.Statement(launchFuncId.Call(launchFuncArgs));
  }

  //oK?
  private XobjList makeLaunchFuncArgs() {
    XobjList launchFuncArgs = Xcons.List();
    for (ACCvar var : _outerVarList) {
      Ident id = var.getId();
      if(var.isArray()){
        Xobject arg0 = id.Ref();
        Ident devicePtr = kernelInfo.getDevicePtr(id.getName());
        Xobject arg1 = devicePtr.Ref();
        launchFuncArgs.add(arg0);
        launchFuncArgs.add(arg1);
      }else{
        if (_readOnlyOuterIdSet.contains(id)) { //is this condition appropriate?
          Ident devicePtrId = kernelInfo.getDevicePtr(var.getName());
          if (devicePtrId != null) {
            launchFuncArgs.add(devicePtrId.Ref());
          } else {
            launchFuncArgs.add(id.Ref());
          }
        } else {
          Xobject arg;
          if (_useMemPoolOuterIdSet.contains(id)) {
            arg = id.getAddr();
          } else {
            Ident devicePtrId = kernelInfo.getDevicePtr(var.getName());
            arg = devicePtrId.Ref();
          }
          launchFuncArgs.add(arg);
        }
      }
      /*
      switch (id.Type().getKind()) {
      case Xtype.ARRAY:
      case Xtype.POINTER: {
        Xobject arg0 = id.Ref();
        Ident devicePtr = kernelInfo.getDevicePtr(id.getName());
        Xobject arg1 = devicePtr.Ref();
        launchFuncArgs.add(arg0);
        launchFuncArgs.add(arg1);
      }
      break;
      case Xtype.BASIC:
      case Xtype.STRUCT:
      case Xtype.UNION:
      case Xtype.ENUM: {
        Ident devicePtrId = null;
        String idName = id.getName();
        if (_readOnlyOuterIdSet.contains(id)) { //is this condition appropriate?
          devicePtrId = kernelInfo.getDevicePtr(idName);
          if (devicePtrId != null) {
            launchFuncArgs.add(devicePtrId.Ref());
          } else {
            launchFuncArgs.add(id.Ref());
          }
        } else {
          Xobject arg;
          if (_useMemPoolOuterIdSet.contains(id)) {
            arg = id.getAddr();
          } else {
            devicePtrId = kernelInfo.getDevicePtr(idName);
            arg = devicePtrId.Ref();
          }
          launchFuncArgs.add(arg);
        }
      }
      break;
      default:
        ACC.fatal("unknown type");
      }
      */
      
      /*
      if(id.Type().isArray()){
        Ident devicePtr = kernelInfo.getDevicePtr(id.getName());
        launchFuncArgs.add(devicePtr);
      }else{
        if(_readOnlyOuterIdSet.contains(id)){
          Ident devicePtr = kernelInfo.getDevicePtr(id.getName());
          if(devicePtr == null){
            launchFuncArgs.add(id.Ref());
          }else{
            launchFuncArgs.add(devicePtr.Ref());
          }
        }else{
          Ident devicePtr = kernelInfo.getDevicePtr(id.getName());
          if(devicePtr == null){
            ACC.fatal("dev ptr not found");
          }
          launchFuncArgs.add(devicePtr.Ref());
        }
      }
      */

    }
    return launchFuncArgs;
  }

  private Ident findInnerBlockIdent(Block topBlock, BlockList body, String name) {
    // if the id exists between topBlock to bb, the id is not outerId
    for (BlockList b_list = body; b_list != null; b_list = b_list.getParentList()) {
      Ident localId = b_list.findLocalIdent(name);
      if (localId != null) return localId;
      if (b_list == topBlock.getParent()) break;
    }
    return null;
  }

  private class DeviceKernelBuildInfo {
    final private List<Block> initBlockList = new ArrayList<Block>();
    final private XobjList paramIdList = Xcons.IDList();
    final private XobjList localIdList = Xcons.IDList();
    public List<Block> getInitBlockList() {return initBlockList;}
    public void addInitBlock(Block b) {initBlockList.add(b);}
    public XobjList getParamIdList() {return paramIdList; }
    public void addParamId(Ident id) {paramIdList.add(id);}
    XobjList getLocalIdList() {return localIdList; }
    public void addLocalId(Ident id) {localIdList.add(id);}
  }

  private XobjectDef makeDeviceKernelDef(String deviceKernelName, List<Ident> outerIdList, List<Block> kernelBody) {
    /* make deviceKernelBody */
    XobjList deviceKernelLocalIds = Xcons.IDList();
    List<Block> initBlocks = new ArrayList<Block>();


    //make params
    XobjList deviceKernelParamIds = Xcons.IDList();
    //add paramId from outerId
    for (Ident id : outerIdList) {
      if (ACC.useReadOnlyDataCache && _readOnlyOuterIdSet.contains(id) && (id.Type().isArray() || id.Type().isPointer())) {
        Xtype constParamType = makeConstRestrictVoidType();
        Ident constParamId = Ident.Param("_ACC_cosnt_" + id.getName(), constParamType);

        Xtype arrayPtrType = Xtype.Pointer(id.Type().getRef());//Xtype.Pointer(id.Type());
        Ident localId = Ident.Local(id.getName(), arrayPtrType);//makeParamId_new(id);
        Xobject initialize = Xcons.Set(localId.Ref(), Xcons.Cast(arrayPtrType, constParamId.Ref()));
        deviceKernelParamIds.add(constParamId);
        deviceKernelLocalIds.add(localId);
        initBlocks.add(Bcons.Statement(initialize));
      } else {
        deviceKernelParamIds.add(makeParamId_new(id));
      }
    }

    DeviceKernelBuildInfo kernelBuildInfo = new DeviceKernelBuildInfo();
    //make mainBody
    Block deviceKernelMainBlock = makeCoreBlock(kernelBody, kernelBuildInfo);

    initBlocks.addAll(kernelBuildInfo.getInitBlockList());

    //add localId, paramId from additional
    deviceKernelLocalIds.mergeList(kernelBuildInfo.getLocalIdList());
    deviceKernelParamIds.mergeList(kernelBuildInfo.getParamIdList());

    //add private varId only if "parallel"
    if (kernelInfo.getPragma() == ACCpragma.PARALLEL) {
      Iterator<ACCvar> varIter = kernelInfo.getVars();
      while (varIter.hasNext()) {
        ACCvar var = varIter.next();
        if (var.isPrivate()) {
          Ident privateId = Ident.Local(var.getName(), var.getId().Type());
          privateId.setProp(ACCgpuDecompiler.GPU_STRAGE_SHARED, true);
          deviceKernelLocalIds.add(privateId);
        }
      }
    }

    //if extern_sm is used, add extern_sm_id & extern_sm_offset_id
    if (sharedMemory.isUsed()) {
      deviceKernelLocalIds.add(sharedMemory.externSmId);
      deviceKernelLocalIds.add(sharedMemory.smOffsetId);
    }

    //if block reduction is used
    deviceKernelLocalIds.mergeList(reductionManager.getBlockReductionLocalIds());
    if (reductionManager.hasUsingTmpReduction()) {
      deviceKernelParamIds.mergeList(reductionManager.getBlockReductionParamIds());
      allocList.add(Xcons.List(reductionManager.tempPtr, Xcons.IntConstant(0), reductionManager.totalElementSize));
    }


    BlockList deviceKernelBody = Bcons.emptyBody();

    //FIXME add extern_sm init func
    if (sharedMemory.isUsed()) {
      deviceKernelBody.add(sharedMemory.makeInitFunc());
    }
    deviceKernelBody.add(reductionManager.makeLocalVarInitFuncs());

    for (Block b : initBlocks) deviceKernelBody.add(b);
    deviceKernelBody.add(deviceKernelMainBlock);

    deviceKernelBody.add(reductionManager.makeReduceAndFinalizeFuncs());

    deviceKernelBody.setIdentList(deviceKernelLocalIds);

    rewriteReferenceType(deviceKernelMainBlock, deviceKernelParamIds);

    Ident deviceKernelId = kernelInfo.getGlobalDecl().getEnvDevice().declGlobalIdent(deviceKernelName, Xtype.Function(Xtype.voidType));
    ((FunctionType) deviceKernelId.Type()).setFuncParamIdList(deviceKernelParamIds);

    return XobjectDef.Func(deviceKernelId, deviceKernelParamIds, null, deviceKernelBody.toXobject());
  }

  private Xtype makeConstRestrictVoidType() {
    Xtype copyType = Xtype.voidType.copy();
    copyType.setIsConst(true);
    Xtype ptrType = Xtype.Pointer(copyType);
    ptrType.setIsRestrict(true);
    return ptrType;
  }

  void rewriteReferenceType(Block b, XobjList paramIds) {
    BasicBlockExprIterator iter = new BasicBlockExprIterator(b);
    for (iter.init(); !iter.end(); iter.next()) {
      Xobject expr = iter.getExpr();
      topdownXobjectIterator exprIter = new topdownXobjectIterator(expr);
      for (exprIter.init(); !exprIter.end(); exprIter.next()) {
        Xobject x = exprIter.getXobject();
        switch (x.Opcode()) {
        case VAR: {
          String varName = x.getName();
          if (varName.startsWith("_ACC_")) break;

          // break if the declaration exists in the DeviceKernelBlock
          if (iter.getBasicBlock().getParent().findVarIdent(varName) != null) break;

          // break if the ident doesn't exist in the parameter list
          Ident id = ACCutil.getIdent(paramIds, varName);
          if (id == null) break;

          // break if type is same
          if (x.Type().equals(id.Type())) break;

          if (id.Type().equals(Xtype.Pointer(x.Type()))) {
            Xobject newXobj = Xcons.PointerRef(id.Ref());
            exprIter.setXobject(newXobj);
          } else {
            ACC.fatal("type mismatch");
          }
        }
        break;
        case VAR_ADDR:
          // need to implement
        {
          Ident id = ACCutil.getIdent(paramIds, x.getName());
          if (id != null) {
            if (!x.Type().equals(Xtype.Pointer(id.Type()))) {
              if (x.Type().equals(id.Type())) {
                Xobject newXobj = id.Ref();
                exprIter.setXobject(newXobj);
              } else {
                ACC.fatal("type mismatch");
              }
            }
          }
        }
        break;
        default:
        }
      }
    }
  }

  private Block makeCoreBlock(Block b, DeviceKernelBuildInfo deviceKernelBuildInfo, String prevExecMethodName) {
    switch (b.Opcode()) {
    case FOR_STATEMENT:
      return makeCoreBlockForStatement((CforBlock) b, deviceKernelBuildInfo, prevExecMethodName);
    case COMPOUND_STATEMENT:
    case ACC_PRAGMA:
      return makeCoreBlock(b.getBody(), deviceKernelBuildInfo, prevExecMethodName);
    case IF_STATEMENT: {
      if (prevExecMethodName == null || prevExecMethodName.equals("block_x")) {
        BlockList resultBody = Bcons.emptyBody();

        Ident sharedIfCond = resultBody.declLocalIdent("_ACC_if_cond", Xtype.charType);
        sharedIfCond.setProp(ACCgpuDecompiler.GPU_STRAGE_SHARED, true);

        Block evalCondBlock = Bcons.IF(
                Xcons.binaryOp(Xcode.LOG_EQ_EXPR, _accThreadIndex, Xcons.IntConstant(0)),
                Bcons.Statement(Xcons.Set(sharedIfCond.Ref(), b.getCondBBlock().toXobject())),
                null);
        Block mainIfBlock = Bcons.IF(
                sharedIfCond.Ref(),
                makeCoreBlock(b.getThenBody(), deviceKernelBuildInfo, prevExecMethodName),
                makeCoreBlock(b.getElseBody(), deviceKernelBuildInfo, prevExecMethodName));

        resultBody.add(evalCondBlock);
        resultBody.add(_accSyncThreads);
        resultBody.add(mainIfBlock);

        return Bcons.COMPOUND(resultBody);
      } else {
        return b.copy();
      }
    }
    default: {
      Block resultBlock = b.copy();
      if (prevExecMethodName == null) {
        resultBlock = Bcons.IF(Xcons.binaryOp(Xcode.LOG_EQ_EXPR, _accBlockIndex, Xcons.IntConstant(0)), resultBlock, Bcons.emptyBlock()); //if(_ACC_block_x_id == 0){b}
      } else if (prevExecMethodName.equals("block_x")) {
        Block ifBlock = Bcons.IF(Xcons.binaryOp(Xcode.LOG_EQ_EXPR, _accThreadIndex, Xcons.IntConstant(0)), resultBlock, null);  //if(_ACC_thread_x_id == 0){b}
        Block syncThreadBlock = ACCutil.createFuncCallBlock("_ACC_GPU_M_BARRIER_THREADS", Xcons.List());
        resultBlock = Bcons.COMPOUND(Bcons.blockList(ifBlock, syncThreadBlock));
      }
      return resultBlock;
    }
    }
  }

  private Block makeCoreBlock(BlockList body, DeviceKernelBuildInfo deviceKernelBuildInfo, String prevExecMethodName) {
    if (body == null) return Bcons.emptyBlock();

    Xobject ids = body.getIdentList();
    Xobject decls = body.getDecls();
    Block varInitSection = null;
    if (prevExecMethodName == null || prevExecMethodName.equals("block_x")) {
      if (ids != null) {
        for (Xobject x : (XobjList) ids) {
          Ident id = (Ident) x;
          id.setProp(ACCgpuDecompiler.GPU_STRAGE_SHARED, true);
        }
      }
      //move decl initializer to body
      if (decls != null) {
        List<Block> varInitBlocks = new ArrayList<Block>();
        for (Xobject x : (XobjList) decls) {
          XobjList decl = (XobjList) x;
          if (decl.right() != null) {
            String varName = decl.left().getString();
            Ident id = ACCutil.getIdent((XobjList) ids, varName);
            Xobject initializer = decl.right();
            decl.setRight(null);
            {
              varInitBlocks.add(Bcons.Statement(Xcons.Set(id.Ref(), initializer)));
            }
          }
        }
        if (!varInitBlocks.isEmpty()) {
          BlockList thenBody = Bcons.emptyBody();
          for (Block b : varInitBlocks) {
            thenBody.add(b);
          }

          Xobject threadIdx = Xcons.Symbol(Xcode.VAR, Xtype.intType, "_ACC_thread_x_id");
          Block ifBlock = Bcons.IF(Xcons.binaryOp(Xcode.LOG_EQ_EXPR, threadIdx, Xcons.IntConstant(0)), Bcons.COMPOUND(thenBody), null);  //if(_ACC_thread_x_id == 0){b}
          Block syncThreadBlock = Bcons.Statement(_accSyncThreads);
          varInitSection = Bcons.COMPOUND(Bcons.blockList(ifBlock, syncThreadBlock));
        }
      }
    }
    BlockList resultBody = Bcons.emptyBody(ids, decls);
    resultBody.add(varInitSection);
    for (Block b = body.getHead(); b != null; b = b.getNext()) {
      resultBody.add(makeCoreBlock(b, deviceKernelBuildInfo, prevExecMethodName));
    }
    return Bcons.COMPOUND(resultBody);
  }

  private Block makeCoreBlock(List<Block> blockList, DeviceKernelBuildInfo deviceKernelBuildInfo) {
    BlockList resultBody = Bcons.emptyBody();
    for (Block b : blockList) {
      resultBody.add(makeCoreBlock(b, deviceKernelBuildInfo, null));
    }
    return makeBlock(resultBody);
  }

  private Block makeBlock(BlockList blockList) {
    if (blockList == null || blockList.isEmpty()) {
      return Bcons.emptyBlock();
    }
    if (blockList.isSingle()) {
      Xobject decls = blockList.getDecls();
      XobjList ids = blockList.getIdentList();
      if ((decls == null || decls.isEmpty()) && (ids == null || ids.isEmpty())) {
        return blockList.getHead();
      }
    }
    return Bcons.COMPOUND(blockList);
  }

  private Block makeCoreBlockForStatement(CforBlock forBlock, DeviceKernelBuildInfo deviceKernelBuildInfo,
                                          String prevExecMethodName) {
    BlockListBuilder resultBlockBuilder = new BlockListBuilder();

    ACCinfo info = ACCutil.getACCinfo(forBlock);
    if (info == null || !info.getPragma().isLoop()) {
      loopStack.push(new Loop(forBlock));
      BlockList body = Bcons.blockList(makeCoreBlock(forBlock.getBody(), deviceKernelBuildInfo , prevExecMethodName));
      loopStack.pop();
      if (prevExecMethodName != null && (prevExecMethodName.equals("thread_x") || prevExecMethodName.equals("block_thread_x"))) {
        return Bcons.FOR(forBlock.getInitBBlock(), forBlock.getCondBBlock(), forBlock.getIterBBlock(), body);
      }
      forBlock.Canonicalize();
      if (forBlock.isCanonical()) {
        Xobject originalInductionVar = forBlock.getInductionVar();
        Ident originalInductionVarId = forBlock.findVarIdent(originalInductionVar.getName());

        BlockList resultBody = Bcons.emptyBody();
        Ident inductionVarId = resultBody.declLocalIdent("_ACC_loop_iter_" + originalInductionVar.getName(),
                originalInductionVar.Type());
        Block mainLoop = Bcons.FOR(Xcons.Set(inductionVarId.Ref(), forBlock.getLowerBound()),
                Xcons.binaryOp(Xcode.LOG_LT_EXPR, inductionVarId.Ref(), forBlock.getUpperBound()),
                Xcons.asgOp(Xcode.ASG_PLUS_EXPR, inductionVarId.Ref(), forBlock.getStep()),
                Bcons.COMPOUND(body));
        Block endIf = Bcons.IF(Xcons.binaryOp(Xcode.LOG_EQ_EXPR, _accThreadIndex, Xcons.IntConstant(0)),
                Bcons.Statement(Xcons.Set(originalInductionVar, inductionVarId.Ref())), null);
        resultBody.add(mainLoop);
        resultBody.add(endIf);
        resultBody.add(_accSyncThreads);

        replaceVar(mainLoop, originalInductionVarId, inductionVarId);
        return Bcons.COMPOUND(resultBody);
      } else {
        ACC.fatal("non canonical loop");
      }
    }

    Xobject numGangsExpr = info.getNumGangsExp();
    Xobject vectorLengthExpr = info.getVectorLengthExp();
//    System.out.println(numGangsExpr);
    if (numGangsExpr != null) gpuManager.setNumGangs(numGangsExpr);
    if (vectorLengthExpr != null) gpuManager.setVectorLength(vectorLengthExpr);

    String execMethodName = gpuManager.getMethodName(forBlock);
    EnumSet<ACCpragma> execMethodSet = gpuManager.getMethodType(forBlock);
    if (execMethodSet.isEmpty()) { //if execMethod is not defined or seq
      loopStack.push(new Loop(forBlock));
      BlockList body = Bcons.blockList(makeCoreBlock(forBlock.getBody(), deviceKernelBuildInfo, prevExecMethodName));
      loopStack.pop();
      return Bcons.FOR(forBlock.getInitBBlock(), forBlock.getCondBBlock(), forBlock.getIterBBlock(), body);
    }

    List<Block> cacheLoadBlocks = new ArrayList<Block>();

    //private
    {
      Iterator<ACCvar> vars = info.getVars();
      while (vars.hasNext()) {
        ACCvar var = vars.next();
        if (!var.isPrivate()) {
          continue;
        }
        Xtype varType = var.getId().Type();
        if (execMethodSet.contains(ACCpragma.VECTOR)) {
          resultBlockBuilder.declLocalIdent(var.getName(), varType);
        } else if (execMethodSet.contains(ACCpragma.GANG)) {
          if (varType.isArray()) {
            Ident arrayPtrId = Ident.Local(var.getName(), Xtype.Pointer(varType.getRef()));
            Ident privateArrayParamId = Ident.Param("_ACC_prv_" + var.getName(), Xtype.voidPtrType);
            deviceKernelBuildInfo.addLocalId(arrayPtrId);
            deviceKernelBuildInfo.addParamId(privateArrayParamId);

            try {
              Xobject sizeObj = Xcons.binaryOp(Xcode.MUL_EXPR,
                      ACCutil.getArrayElmtCountObj(varType),
                      Xcons.SizeOf(varType.getArrayElementType()));
              XobjList initPrivateFuncArgs = Xcons.List(Xcons.Cast(Xtype.Pointer(Xtype.voidPtrType), arrayPtrId.getAddr()), privateArrayParamId.Ref(), sizeObj);
              Block initPrivateFuncCall = ACCutil.createFuncCallBlock("_ACC_init_private", initPrivateFuncArgs);
              deviceKernelBuildInfo.addInitBlock(initPrivateFuncCall);
              allocList.add(Xcons.List(var.getId(), Xcons.IntConstant(0), sizeObj));
            } catch (Exception e) {
              ACC.fatal(e.getMessage());
            }
          } else {
            Ident privateLocalId = Ident.Local(var.getName(), varType);
            privateLocalId.setProp(ACCgpuDecompiler.GPU_STRAGE_SHARED, true);
            deviceKernelBuildInfo.addLocalId(privateLocalId);
          }
        }
      }
    }

    //begin reduction
    List<Reduction> reductionList = new ArrayList<Reduction>();
    Iterator<ACCvar> vars = info.getVars();
    while (vars.hasNext()) {
      ACCvar var = vars.next();
      if (!var.isReduction()) continue;

      Reduction reduction = reductionManager.addReduction(var, execMethodSet);
      if (!_readOnlyOuterIdSet.contains(var.getId()) && !execMethodSet.contains(ACCpragma.GANG)) {
        // only thread reduction
        resultBlockBuilder.addIdent(reduction.getLocalReductionVarId());
        resultBlockBuilder.addInitBlock(reduction.makeInitReductionVarFuncCall());
        resultBlockBuilder.addFinalizeBlock(reduction.makeThreadReductionFuncCall());
      }
      reductionList.add(reduction);
    }//end reduction


    LinkedList<CforBlock> collapsedForBlockList = new LinkedList<CforBlock>();
    collapsedForBlockList.add(forBlock);

    {
      CforBlock tmpForBlock = forBlock;
      for (int i = 1; i < info.getCollapseNum(); i++) {
        tmpForBlock = (CforBlock) tmpForBlock.getBody().getHead();
        collapsedForBlockList.add(tmpForBlock);
      }
    }

    //make calc idx funcs
    List<Block> calcIdxFuncCalls = new ArrayList<Block>();
    XobjList vIdxIdList = Xcons.IDList();
    XobjList nIterIdList = Xcons.IDList();
    XobjList indVarIdList = Xcons.IDList();
    Boolean has64bitIndVar = false;
    for (CforBlock tmpForBlock : collapsedForBlockList) {
      String indVarName = tmpForBlock.getInductionVar().getName();
      Xtype indVarType = tmpForBlock.findVarIdent(indVarName).Type();
      Xtype idxVarType = Xtype.unsignedType;
      switch (indVarType.getBasicType()) {
      case BasicType.INT:
      case BasicType.UNSIGNED_INT:
        idxVarType = Xtype.unsignedType;
        break;
      case BasicType.LONGLONG:
      case BasicType.UNSIGNED_LONGLONG:
        idxVarType = Xtype.unsignedlonglongType;
        has64bitIndVar = true;
        break;
      }
      Xobject init = tmpForBlock.getLowerBound();
      Xobject cond = tmpForBlock.getUpperBound();
      Xobject step = tmpForBlock.getStep();
      Ident vIdxId = Ident.Local("_ACC_idx_" + indVarName, idxVarType);
      Ident indVarId = Ident.Local(indVarName, indVarType);
      Ident nIterId = resultBlockBuilder.declLocalIdent("_ACC_niter_" + indVarName, idxVarType);
      Block calcNiterFuncCall = ACCutil.createFuncCallBlock("_ACC_calc_niter", Xcons.List(nIterId.getAddr(), init, cond, step));
      Block calcIdxFuncCall = ACCutil.createFuncCallBlock("_ACC_gpu_calc_idx", Xcons.List(vIdxId.Ref(), indVarId.getAddr(), init, cond, step));

      resultBlockBuilder.addInitBlock(calcNiterFuncCall);

      vIdxIdList.add(vIdxId);
      nIterIdList.add(nIterId);
      indVarIdList.add(indVarId);
      calcIdxFuncCalls.add(calcIdxFuncCall);
    }

    Xtype globalIdxType = has64bitIndVar ? Xtype.unsignedlonglongType : Xtype.unsignedType;

    Ident iterIdx = resultBlockBuilder.declLocalIdent("_ACC_" + execMethodName + "_idx", globalIdxType);
    Ident iterInit = resultBlockBuilder.declLocalIdent("_ACC_" + execMethodName + "_init", globalIdxType);
    Ident iterCond = resultBlockBuilder.declLocalIdent("_ACC_" + execMethodName + "_cond", globalIdxType);
    Ident iterStep = resultBlockBuilder.declLocalIdent("_ACC_" + execMethodName + "_step", globalIdxType);

    XobjList initIterFuncArgs = Xcons.List(iterInit.getAddr(), iterCond.getAddr(), iterStep.getAddr());
    Xobject nIterAll = Xcons.IntConstant(1);
    for (Xobject x : nIterIdList) {
      Ident nIterId = (Ident) x;
      nIterAll = Xcons.binaryOp(Xcode.MUL_EXPR, nIterAll, nIterId.Ref());
    }
    initIterFuncArgs.add(nIterAll);

    Block initIterFunc = ACCutil.createFuncCallBlock("_ACC_gpu_init_" + execMethodName + "_iter", initIterFuncArgs);
    resultBlockBuilder.addInitBlock(initIterFunc);


    //make clac each idx from virtual idx
    Block calcEachVidxBlock = makeCalcIdxFuncCall(vIdxIdList, nIterIdList, iterIdx);


    //push Loop to stack
    Loop thisLoop = new Loop(forBlock);
    thisLoop.setAbstractIter(iterIdx, iterInit, iterCond, iterStep);
    loopStack.push(thisLoop);

    List<Cache> cacheList = new ArrayList<Cache>();

    //begin cache
    {
      Block headBlock = forBlock.getBody().getHead();
      if (headBlock != null && headBlock.Opcode() == Xcode.ACC_PRAGMA) {
        ACCinfo headInfo = ACCutil.getACCinfo(headBlock);
        if (headInfo.getPragma() == ACCpragma.CACHE) {
          Iterator<ACCvar> varIter = headInfo.getVars();
          while (varIter.hasNext()) {
            ACCvar var = varIter.next();
            if (!var.isCache()) continue;


            Ident cachedId = var.getId();
            XobjList subscripts = var.getSubscripts();

            Cache cache = sharedMemory.alloc(cachedId, subscripts);

            resultBlockBuilder.addInitBlock(cache.initFunc);
            cacheLoadBlocks.add(cache.loadBlock);

            resultBlockBuilder.addIdent(cache.cacheId);
            resultBlockBuilder.addIdent(cache.cacheSizeArrayId);
            resultBlockBuilder.addIdent(cache.cacheOffsetArrayId);

            //for after rewrite
            cacheList.add(cache);
          }//end while
        }
      }
    }
    //end cache

    BlockList parallelLoopBody = Bcons.emptyBody();
    parallelLoopBody.add(calcEachVidxBlock);

    for (Block b : calcIdxFuncCalls) parallelLoopBody.add(b);

    // add cache load funcs
    for (Block b : cacheLoadBlocks) {
      parallelLoopBody.add(b);
    }
    // add inner block
    BlockList innerBody = collapsedForBlockList.getLast().getBody();
    Block coreBlock = makeCoreBlock(innerBody, deviceKernelBuildInfo, execMethodName);

    //rewirteCacheVars
    for(Cache cache : cacheList){
      cache.rewrite(coreBlock);
    }
    parallelLoopBody.add(coreBlock);

    //add the cache barrier func
    if (!cacheLoadBlocks.isEmpty()) {
      parallelLoopBody.add(_accSyncThreads);
    }

    {
      XobjList forBlockListIdents = (XobjList) indVarIdList.copy();//Xcons.List(indVarId);
      forBlockListIdents.mergeList(vIdxIdList);
      ///insert
      parallelLoopBody.setIdentList(forBlockListIdents);
    }

    Block parallelLoopBlock = Bcons.FOR(
            Xcons.Set(iterIdx.Ref(), iterInit.Ref()),
            Xcons.binaryOp(Xcode.LOG_LT_EXPR, iterIdx.Ref(), iterCond.Ref()),
            Xcons.asgOp(Xcode.ASG_PLUS_EXPR, iterIdx.Ref(), iterStep.Ref()),
            Bcons.COMPOUND(parallelLoopBody)
    );

    //rewriteReductionvar
    for (Reduction red : reductionList) {
      red.rewrite(parallelLoopBlock);
    }


    //make resultBody
    resultBlockBuilder.add(parallelLoopBlock);

    if (execMethodSet.contains(ACCpragma.VECTOR)) {
      resultBlockBuilder.addFinalizeBlock(Bcons.Statement(_accSyncThreads));
    }

    //pop stack
    loopStack.pop();

    BlockList resultBody = resultBlockBuilder.build();
    return Bcons.COMPOUND(resultBody);
  }


  private Block makeCalcIdxFuncCall(XobjList vidxIdList, XobjList nIterIdList, Ident vIdx) {
    int i;
    Xobject result = vIdx.Ref();
    Ident funcId = ACCutil.getMacroFuncId("_ACC_calc_vidx", Xtype.intType);

    for (i = vidxIdList.Nargs() - 1; i > 0; i--) {
      Ident indVarId = (Ident) (vidxIdList.getArg(i));
      Ident nIterId = (Ident) (nIterIdList.getArg(i));
      Block callBlock = Bcons.Statement(funcId.Call(Xcons.List(indVarId.getAddr(), nIterId.Ref(), result)));
      result = callBlock.toXobject();
    }

    Ident indVarId = (Ident) (vidxIdList.getArg(0));
    result = Xcons.Set(indVarId.Ref(), result);

    return Bcons.Statement(result);
  }



  private XobjectDef makeLaunchFuncDef(String launchFuncName, XobjectDef deviceKernelDef) {
    XobjList launchFuncParamIds = Xcons.IDList();
    XobjList deviceKernelCallArgs = Xcons.List();
    BlockListBuilder blockListBuilder = new BlockListBuilder();

    //# of block and thread
    Ident blockXid = blockListBuilder.declLocalIdent("_ACC_GPU_DIM3_block_x", Xtype.intType);
    Ident blockYid = blockListBuilder.declLocalIdent("_ACC_GPU_DIM3_block_y", Xtype.intType);
    Ident blockZid = blockListBuilder.declLocalIdent("_ACC_GPU_DIM3_block_z", Xtype.intType);
    Ident threadXid = blockListBuilder.declLocalIdent("_ACC_GPU_DIM3_thread_x", Xtype.intType);
    Ident threadYid = blockListBuilder.declLocalIdent("_ACC_GPU_DIM3_thread_y", Xtype.intType);
    Ident threadZid = blockListBuilder.declLocalIdent("_ACC_GPU_DIM3_thread_z", Xtype.intType);

    XobjList blockThreadSize = gpuManager.getBlockThreadSize();
    XobjList blockSize = (XobjList) blockThreadSize.left();
    XobjList threadSize = (XobjList) blockThreadSize.right();

    blockListBuilder.addInitBlock(Bcons.Statement(Xcons.Set(blockXid.Ref(), blockSize.getArg(0))));
    blockListBuilder.addInitBlock(Bcons.Statement(Xcons.Set(blockYid.Ref(), blockSize.getArg(1))));
    blockListBuilder.addInitBlock(Bcons.Statement(Xcons.Set(blockZid.Ref(), blockSize.getArg(2))));
    blockListBuilder.addInitBlock(Bcons.Statement(Xcons.Set(threadXid.Ref(), threadSize.getArg(0))));
    blockListBuilder.addInitBlock(Bcons.Statement(Xcons.Set(threadYid.Ref(), threadSize.getArg(1))));
    blockListBuilder.addInitBlock(Bcons.Statement(Xcons.Set(threadZid.Ref(), threadSize.getArg(2))));

    Xobject max_num_grid = Xcons.IntConstant(65535);
    Block adjustGridFuncCall = ACCutil.createFuncCallBlock("_ACC_GPU_ADJUST_GRID", Xcons.List(Xcons.AddrOf(blockXid.Ref()), Xcons.AddrOf(blockYid.Ref()), Xcons.AddrOf(blockZid.Ref()), max_num_grid));
    blockListBuilder.addInitBlock(adjustGridFuncCall);


    Ident mpool = Ident.Local("_ACC_GPU_mpool", Xtype.voidPtrType);
    Ident mpoolPos = Ident.Local("_ACC_GPU_mpool_pos", Xtype.longlongType);

    if (!allocList.isEmpty() || !_useMemPoolOuterIdSet.isEmpty()) {
      Block initMpoolPos = Bcons.Statement(Xcons.Set(mpoolPos.Ref(), Xcons.LongLongConstant(0, 0)));
      Block getMpoolFuncCall = null;
      try {
        if (kernelInfo.isAsync() && kernelInfo.getAsyncExp() != null) {
          getMpoolFuncCall = ACCutil.createFuncCallBlock("_ACC_gpu_mpool_get_async", Xcons.List(mpool.getAddr(), kernelInfo.getAsyncExp()));
        } else {
          getMpoolFuncCall = ACCutil.createFuncCallBlock("_ACC_gpu_mpool_get", Xcons.List(mpool.getAddr()));
        }
      } catch (ACCexception e) {
        ACC.fatal(e.getMessage());
      }
      blockListBuilder.addIdent(mpool);
      blockListBuilder.addIdent(mpoolPos);
      blockListBuilder.addInitBlock(initMpoolPos);
      blockListBuilder.addInitBlock(getMpoolFuncCall);
    }

    XobjList reductionKernelCallArgs = Xcons.List();
    int reductionKernelVarCount = 0;
    for (Ident varId : _outerIdList) {
      Xobject paramRef;
      if (_useMemPoolOuterIdSet.contains(varId)) {
        Ident paramId = Ident.Param(varId.getName(), Xtype.Pointer(varId.Type()));
        launchFuncParamIds.add(paramId);

        Ident devPtrId = blockListBuilder.declLocalIdent("_ACC_gpu_dev_" + varId.getName(), Xtype.voidPtrType);

        ACCvar var = kernelInfo.getACCvar(varId);
        Xobject size = var.getSize();

        Block mpoolAllocFuncCall = ACCutil.createFuncCallBlock("_ACC_gpu_mpool_alloc", Xcons.List(devPtrId.getAddr(), size, mpool.Ref(), mpoolPos.getAddr()));
        Block mpoolFreeFuncCall = ACCutil.createFuncCallBlock("_ACC_gpu_mpool_free", Xcons.List(devPtrId.Ref(), mpool.Ref()));
        Block HtoDCopyFuncCall = ACCutil.createFuncCallBlock("_ACC_gpu_copy", Xcons.List(paramId.Ref(), devPtrId.Ref(), size, Xcons.IntConstant(400)));
        Block DtoHCopyFuncCall = ACCutil.createFuncCallBlock("_ACC_gpu_copy", Xcons.List(paramId.Ref(), devPtrId.Ref(), size, Xcons.IntConstant(401)));
        blockListBuilder.addInitBlock(mpoolAllocFuncCall);
        blockListBuilder.addInitBlock(HtoDCopyFuncCall);
        blockListBuilder.addFinalizeBlock(DtoHCopyFuncCall);
        blockListBuilder.addFinalizeBlock(mpoolFreeFuncCall);
        paramRef = Xcons.Cast(Xtype.Pointer(varId.Type()), devPtrId.Ref());
      } else {
        Ident paramId = makeParamId_new(varId);
        switch (varId.Type().getKind()) {
        case Xtype.ARRAY:
        case Xtype.POINTER:
          launchFuncParamIds.add(paramId);
          Ident paramDevId = Ident.Param("_ACC_gpu_dev_" + paramId.getName(), paramId.Type());
          launchFuncParamIds.add(paramDevId);
          paramRef = paramDevId.Ref();
          break;
        default:
          launchFuncParamIds.add(paramId);
          paramRef = paramId.Ref();
        }
      }

      deviceKernelCallArgs.add(paramRef);
      {
        Reduction red = reductionManager.findReduction(varId);
        if (red != null && red.useBlock() && red.usesTmp()) {
          reductionKernelCallArgs.add(paramRef);
          reductionKernelVarCount++;
        }
      }
    }

    for (XobjList xobjList : allocList) {
      Ident varId = (Ident) (xobjList.getArg(0));
      Xobject baseSize = xobjList.getArg(1);
      Xobject numBlocksFactor = xobjList.getArg(2);

      Ident devPtrId = blockListBuilder.declLocalIdent("_ACC_gpu_device_" + varId.getName(), Xtype.voidPtrType);
      deviceKernelCallArgs.add(devPtrId.Ref());
      if (varId.getName().equals(ACC_REDUCTION_TMP_VAR)) {
        reductionKernelCallArgs.add(devPtrId.Ref());
      }

      Xobject size = Xcons.binaryOp(Xcode.PLUS_EXPR, baseSize,
              Xcons.binaryOp(Xcode.MUL_EXPR, numBlocksFactor, blockXid.Ref()));
      Block mpoolAllocFuncCall = ACCutil.createFuncCallBlock("_ACC_gpu_mpool_alloc", Xcons.List(devPtrId.getAddr(), size, mpool.Ref(), mpoolPos.getAddr()));
      Block mpoolFreeFuncCall = ACCutil.createFuncCallBlock("_ACC_gpu_mpool_free", Xcons.List(devPtrId.Ref(), mpool.Ref()));
      blockListBuilder.addInitBlock(mpoolAllocFuncCall);
      blockListBuilder.addFinalizeBlock(mpoolFreeFuncCall);
    }

    //add blockReduction cnt & tmp
    if (reductionManager.hasUsingTmpReduction()) {
      Ident blockCountId = blockListBuilder.declLocalIdent("_ACC_gpu_block_count", Xtype.Pointer(Xtype.unsignedType));
      deviceKernelCallArgs.add(blockCountId.Ref());
      Block getBlockCounterFuncCall = null;
      try {
        if (kernelInfo.isAsync() && kernelInfo.getAsyncExp() != null) {
          getBlockCounterFuncCall = ACCutil.createFuncCallBlock("_ACC_gpu_get_block_count_async", Xcons.List(blockCountId.getAddr(), kernelInfo.getAsyncExp()));
        } else {
          getBlockCounterFuncCall = ACCutil.createFuncCallBlock("_ACC_gpu_get_block_count", Xcons.List(blockCountId.getAddr()));
        }
      } catch (ACCexception e) {
        ACC.fatal(e.getMessage());
      }
      blockListBuilder.addInitBlock(getBlockCounterFuncCall);
    }

    Ident deviceKernelId = (Ident) deviceKernelDef.getNameObj();
    Xobject deviceKernelCall = deviceKernelId.Call(deviceKernelCallArgs);
    //FIXME merge GPU_FUNC_CONF and GPU_FUNC_CONF_ASYNC
    deviceKernelCall.setProp(ACCgpuDecompiler.GPU_FUNC_CONF, Xcons.List(blockXid, blockYid, blockZid, threadXid, threadYid, threadZid));
    if (kernelInfo.isAsync()) {
      try {
        Xobject asyncExp = kernelInfo.getAsyncExp();
        if (asyncExp != null) {
          deviceKernelCall.setProp(ACCgpuDecompiler.GPU_FUNC_CONF_ASYNC, Xcons.List(asyncExp));
        } else {
          deviceKernelCall.setProp(ACCgpuDecompiler.GPU_FUNC_CONF_ASYNC, Xcons.List(Xcons.IntConstant(ACC.ACC_ASYNC_NOVAL)));
        }
      } catch (Exception e) {
        ACC.fatal("can't set async prop");
      }
    } else {
      deviceKernelCall.setProp(ACCgpuDecompiler.GPU_FUNC_CONF_ASYNC, Xcons.List(Xcons.IntConstant(ACC.ACC_ASYNC_SYNC)));
    }

    if (sharedMemory.isUsed()) {
      Xobject maxSmSize = sharedMemory.getMaxSize();
      deviceKernelCall.setProp(ACCgpuDecompiler.GPU_FUNC_CONF_SHAREDMEMORY, maxSmSize);
    }

    blockListBuilder.add(Bcons.Statement(deviceKernelCall));

    if (reductionManager.hasUsingTmpReduction()) {
      XobjectDef reductionKernelDef = reductionManager.makeReductionKernelDef(launchFuncName + "_red" + ACC_GPU_DEVICE_FUNC_SUFFIX);
      Ident reductionKernelId = (Ident) reductionKernelDef.getNameObj();
      reductionKernelCallArgs.add(blockXid.Ref());
      Xobject reductionKernelCall = reductionKernelId.Call(reductionKernelCallArgs);

      Xobject constant1 = Xcons.IntConstant(1);
      reductionKernelCall.setProp(ACCgpuDecompiler.GPU_FUNC_CONF, Xcons.List(Xcons.IntConstant(reductionKernelVarCount), constant1, constant1, Xcons.IntConstant(256), constant1, constant1));
      Object asyncObj = null;
      if (kernelInfo.isAsync()) {
        try {
          Xobject asyncExp = kernelInfo.getAsyncExp();
          if (asyncExp != null) {
            asyncObj = Xcons.List(asyncExp);
          } else {
            asyncObj = Xcons.List(Xcons.IntConstant(ACC.ACC_ASYNC_NOVAL));
          }
        } catch (Exception e) {
          ACC.fatal("can't set async prop");
        }
      } else {
        asyncObj = Xcons.List(Xcons.IntConstant(ACC.ACC_ASYNC_SYNC));
      }
      reductionKernelCall.setProp(ACCgpuDecompiler.GPU_FUNC_CONF_ASYNC, asyncObj);
      XobjectFile devEnv = kernelInfo.getGlobalDecl().getEnvDevice();
      devEnv.add(reductionKernelDef);
      Block ifBlock = Bcons.IF(Xcons.binaryOp(Xcode.LOG_GT_EXPR, blockXid.Ref(), Xcons.IntConstant(1)), Bcons.Statement(reductionKernelCall), null);
      blockListBuilder.add(ifBlock);
    }

    if (!kernelInfo.isAsync()) {
      blockListBuilder.add(ACCutil.createFuncCallBlock("_ACC_gpu_wait", Xcons.List(_accAsyncSync)));
    }

    BlockList launchFuncBody = blockListBuilder.build();

    ACCglobalDecl globalDecl = kernelInfo.getGlobalDecl();
    Ident launchFuncId = globalDecl.getEnvDevice().declGlobalIdent(launchFuncName, Xtype.Function(Xtype.voidType));

    return XobjectDef.Func(launchFuncId, launchFuncParamIds, null, launchFuncBody.toXobject());
  }

  private Ident makeParamId_new(Ident id) {
    String varName = id.getName();

    switch (id.Type().getKind()) {
    case Xtype.ARRAY:
    case Xtype.POINTER: {
      return Ident.Local(varName, id.Type());
    }
    case Xtype.BASIC:
    case Xtype.STRUCT:
    case Xtype.UNION:
    case Xtype.ENUM: {
      // check whether id is firstprivate!
      if (kernelInfo.isVarAllocated(varName)) {
        return Ident.Local(varName, Xtype.Pointer(id.Type()));
      } else {
        if (_useMemPoolOuterIdSet.contains(id)) {
          return Ident.Local(varName, Xtype.Pointer(id.Type()));
        }
        return Ident.Local(varName, id.Type());
      }
    }
    default:
      ACC.fatal("unknown type");
      return null;
    }
  }


  public void analyze() {
    if (_kernelBlocks.size() == 1) { // implement for kernel that has more than one block.
      Block kernelblock = _kernelBlocks.get(0);
      analyzeKernelBlock(kernelblock);
    }

    gpuManager.analyze();

    //get outerId set
    Set<Ident> outerIdSet = new HashSet<Ident>();
    //OuterIdCollector outerIdCollector = new OuterIdCollector();
    OuterIdCollector outerIdCollector = new OuterIdCollector();
    for (Block b : _kernelBlocks) {
      Set<Ident> blockouterIdSet = outerIdCollector.collect(b); //collect(b); // = collect(b,b);
      //blockouterIdSet.removeAll(collectPrivatizedIdSet(b));
      blockouterIdSet.removeAll(collectInductionVarIdSet(b));

      //blockouterIdSet.removeAll(_inductionVarIds);
      outerIdSet.addAll(blockouterIdSet);
    }
    //remove private id
//    Iterator<ACCvar> varIter = kernelInfo.getVars();
//    while (varIter.hasNext()) {
//      ACCvar var = varIter.next();
//      if (var.isPrivate()) {
//        outerIdSet.remove(var.getId());
//      }
//    }

    //make outerId list
    _outerIdList = new ArrayList<Ident>(outerIdSet);


    //collect read only id
    _readOnlyOuterIdSet = collectReadOnlyId(_kernelBlocks, _outerIdList); //collectReadOnlyIdSet();

    //collect private var ids
    //Set<Ident> _inductionVarIdSet = collectInductionVarSet();

    //useMemPoolVarId
    for (Ident id : _outerIdList) {
      if (kernelInfo.isVarReduction(id.getName())) {
        if (!kernelInfo.isVarAllocated(id)) {
          _useMemPoolOuterIdSet.add(id);
        }
      }
    }
  }

  private Set<Ident> collectInductionVarIdSet(Block kernelBlock) { //rename to collectInductionVarIds
    Set<Ident> indVarIdSet = new HashSet<Ident>();

    topdownBlockIterator blockIter = new topdownBlockIterator(kernelBlock);
    for (blockIter.init(); !blockIter.end(); blockIter.next()) {
      Block b = blockIter.getBlock();
      if (b.Opcode() != Xcode.FOR_STATEMENT) continue;
      ACCinfo info = ACCutil.getACCinfo(b);
      if (info == null) continue;
      CforBlock forBlock = (CforBlock) b;
      if (gpuManager.getMethodName(forBlock).isEmpty()) continue;
      for (int i = info.getCollapseNum(); i > 0; --i) {
        Ident indVarId = b.findVarIdent(forBlock.getInductionVar().getName());
        indVarIdSet.add(indVarId);
        if (i > 1) {
          forBlock = (CforBlock) (forBlock.getBody().getHead());
        }
      }
    }

    return indVarIdSet;
  }

  private void analyzeKernelBlock(Block block) {
    topdownBlockIterator blockIter = new topdownBlockIterator(block);
    for (blockIter.init(); !blockIter.end(); blockIter.next()) {
      Block b = blockIter.getBlock();
      if (b.Opcode() != Xcode.ACC_PRAGMA) continue;

      ACCinfo info = ACCutil.getACCinfo(b);
      if (info.getPragma().isLoop()) {
        CforBlock forBlock = (CforBlock) b.getBody().getHead();
        ACCutil.setACCinfo(forBlock, info);
        Iterator<ACCpragma> execModelIter = info.getExecModels();
        gpuManager.addLoop(execModelIter, forBlock);
      }
    }
  }

  public List<Ident> getOuterIdList() {
    return _outerIdList;
  }

  private void replaceVar(Block b, Ident fromId, Ident toId) {
    BasicBlockExprIterator iter = new BasicBlockExprIterator(b);
    for (iter.init(); !iter.end(); iter.next()) {
      Xobject expr = iter.getExpr();
      topdownXobjectIterator exprIter = new topdownXobjectIterator(expr);
      for (exprIter.init(); !exprIter.end(); exprIter.next()) {
        Xobject x = exprIter.getXobject();
        if (x.Opcode() == Xcode.VAR) {
          String varName = x.getName();
          if (fromId.getName().equals(varName)) {
            Ident id = findInnerBlockIdent(b, iter.getBasicBlock().getParent().getParent(), varName);
            if (id == null) {
              exprIter.setXobject(toId.Ref());
            }
          }
        }
      }
    }
  }

  private Xobject getAssignedXobject(Xobject x) throws ACCexception {
    switch (x.Opcode()) {
    case VAR:
    case ARRAY_ADDR:
    case INT_CONSTANT:
      return x;
    case ARRAY_REF:
    case MEMBER_REF:
    case ADDR_OF:
    case POINTER_REF:
    case CAST_EXPR:
      return getAssignedXobject(x.getArg(0));
    case PLUS_EXPR:
    case MINUS_EXPR: {
      //only for pointer operation
      if (! x.Type().isPointer()) throw new ACCexception("not pointer");
      Xobject lhs = x.getArg(0);
      Xobject rhs = x.getArg(1);
      if(lhs.Type().isPointer()){
        return getAssignedXobject(lhs);
      }else if(rhs.Type().isPointer()){
        return getAssignedXobject(rhs);
      }else{
        throw new ACCexception("no pointer operand for PLUS or MINUS");
      }
    }
    case FUNCTION_CALL: {
      Xobject funcAddr = x.getArg(0);
      // for arrayRef generated by XMP
      if (funcAddr.getName().startsWith("_XMP_M_GET_ADDR_E")) {
        Xobject args = x.getArg(1);
        return args.getArg(0);
      }
    }
    default:
      throw new ACCexception("not supported type: " + x.Opcode());
    }
  }
  private Ident getAssignedId(Xobject expr, Block b){
    Ident id = null;
    try{
      Xobject assignedXobjecct = getAssignedXobject(expr);
      String varName = assignedXobjecct.getName();
      id = b.findVarIdent(varName);
      if(id == null){
        throw new ACCexception("variable '" + varName + "' is not found");
      }
    }catch(ACCexception accException){
      ACC.fatal("getAssignedId: " + accException.getMessage());
    }
    return id;
  }
  private Xobject findAssignedExpr(Xobject x){
    switch (x.Opcode()) {
    case PRE_INCR_EXPR:
    case PRE_DECR_EXPR:
    case POST_INCR_EXPR:
    case POST_DECR_EXPR:
      return x.getArg(0);
    default:
      if (x.Opcode().isAsgOp()) {
        return x.getArg(0);
      }else{
        return null;
      }
    }
  }
  private Set<Ident> collectAssignedId(Xobject expr, Block b){
    Set<Ident> assignedIds = new LinkedHashSet<Ident>();
    XobjectIterator xobjIter = new topdownXobjectIterator(expr);
    for (xobjIter.init(); !xobjIter.end(); xobjIter.next()) {
      Xobject x = xobjIter.getXobject();
      if(x == null) continue;
      Xobject assignedExpr = findAssignedExpr(x);
      if(assignedExpr == null) continue;
      Ident assignedVarId = getAssignedId(assignedExpr, b);
      assignedIds.add(assignedVarId);
    }
    return assignedIds;
  }
  private Set<Ident> collectReadOnlyId(List<Block> kernelBlocks, List<Ident> outerIds) {
    Set<Ident> readOnlyIds = new LinkedHashSet<Ident>(outerIds);

    for (Block kernelBlock : kernelBlocks) {
      BasicBlockExprIterator bbExprIter = new BasicBlockExprIterator(kernelBlock);
      for (bbExprIter.init(); !bbExprIter.end(); bbExprIter.next()) {
        Set<Ident> assignedIds = collectAssignedId(bbExprIter.getExpr(), bbExprIter.getBasicBlock().getParent());
        readOnlyIds.removeAll(assignedIds);
      }

      BlockIterator bIter = new topdownBlockIterator(kernelBlock);
      for(bIter.init(); !bIter.end(); bIter.next()) {
        Block b = bIter.getBlock();
        if (b.Opcode() != Xcode.COMPOUND_STATEMENT) continue;
        Xobject decls = b.getBody().getDecls();
        if (decls == null) continue;
        Set<Ident> assignedIdsInDecl = collectAssignedId(decls, b);
        readOnlyIds.removeAll(assignedIdsInDecl);
      }
    }

    return readOnlyIds;
  }

  public void setReadOnlyOuterIdSet(Set<Ident> readOnlyOuterIdSet) {
    _readOnlyOuterIdSet = readOnlyOuterIdSet;
  }

  public Set<Ident> getReadOnlyOuterIdSet() {
    return _readOnlyOuterIdSet;
  }

  public Set<Ident> getOuterIdSet() {
    return new HashSet<Ident>(_outerIdList);
  }

  private class OuterIdCollector {
    public Set<Ident> collect(Block topBlock) {
      Set<Ident> outerIdSet = new LinkedHashSet<Ident>();

      collectVarIdents(topBlock, outerIdSet);
      collectlVarIdentsInDecl(topBlock, outerIdSet);

      return  outerIdSet;
    }

    private void collectlVarIdentsInDecl(Block topBlock, Set<Ident> outerIdSet) {
      BlockIterator bIter = new topdownBlockIterator(topBlock);
      for(bIter.init(); !bIter.end(); bIter.next()) {
        Block b = bIter.getBlock();
        if (b.Opcode() != Xcode.COMPOUND_STATEMENT) continue;
        Xobject decls = b.getBody().getDecls();
        if (decls == null) continue;
        Set<String> varNameSet = collectVarNames(decls);
        for (String name : varNameSet) {
          Ident id = find(topBlock, b, name);
          if (id == null) continue;
          outerIdSet.add(id);
        }
      }
    }

    private void collectVarIdents(Block topBlock, Set<Ident> outerIdSet) {
      BasicBlockExprIterator bbexprIter = new BasicBlockExprIterator(topBlock);
      for(bbexprIter.init(); !bbexprIter.end(); bbexprIter.next()){
        for(String varName : collectVarNames(bbexprIter.getExpr())){
          Ident id = find(topBlock, bbexprIter.getBasicBlock().getParent(), varName);
          if (id == null) continue;
          outerIdSet.add(id);
        }
      }
    }

    private Set<String> collectVarNames(Xobject expr) {
      Set<String> varNameSet = new LinkedHashSet<String>();

      XobjectIterator xobjIter = new topdownXobjectIterator(expr);
      for (xobjIter.init(); !xobjIter.end(); xobjIter.next()) {
        Xobject x = xobjIter.getXobject();
        if (x == null) continue;
        switch (x.Opcode()) {
        case VAR: {
          String varName = x.getName();
          varNameSet.add(varName);
        }
        break;
        case ARRAY_REF: {
          String arrayName = x.getArg(0).getName();
          varNameSet.add(arrayName);
        }
        break;
        default:
        }
      }
      return varNameSet;
    }

    private boolean isPrivate(PragmaBlock pb, String varName){
      ACCinfo info = ACCutil.getACCinfo(pb);
      if(info == null) return false;

      if(info.isVarPrivate(varName)) return true;

      //add loop induction var check here
//      if(info.getPragma().isLoop()){
//        CforBlock forBlock = (CforBlock)info.getBlock().getBody().getHead();
//        Xobject indVar = forBlock.getInductionVar();
//        if(indVar.getName().equals(varName)){
//          return true;
//        }
//      }

      return false;
    }

    private Ident find(Block topBlock, Block block, String name) {
      // if an id exists between root to topBlock, the id is outerId
      for(Block b = block; b != null; b = b.getParentBlock()){
        if(b == topBlock.getParentBlock()) break;
        if(b.Opcode() == Xcode.ACC_PRAGMA && isPrivate((PragmaBlock) b, name)) return null;
        if(hasLocalIdent(b.getBody(),name)) return null;
      }
      return topBlock.findVarIdent(name);
    }

    private boolean hasLocalIdent(BlockList body, String varName) {
      return body != null && body.findLocalIdent(varName) != null;
    }
  }

  class Loop {
    final CforBlock forBlock;
    /*
    Xobject init;
    Xobject cond;
    Xobject step;
    Xobject ind;*/
    boolean isParallelized;
    Ident abstIdx;
    Ident abstCond;
    Ident abstStep;

    Loop(CforBlock forBlock) {
      this.forBlock = forBlock;
    }

    void setAbstractIter(Ident idx, Ident init, Ident cond, Ident step) {
      abstIdx = idx;
      abstCond = cond;
      abstStep = step;
      isParallelized = true;
    }
  }

  class SharedMemory {
    final Ident externSmId;
    final Ident smOffsetId;
    final ArrayDeque<Xobject> smStack = new ArrayDeque<Xobject>();
    Xobject maxSize = Xcons.IntConstant(0);

    boolean isUsed = false;

    SharedMemory() {
      Xtype externSmType = Xtype.Array(Xtype.charType, null);
      externSmId = Ident.Var("_ACC_sm", externSmType, Xtype.Pointer(externSmType), VarScope.GLOBAL);
      externSmId.setStorageClass(StorageClass.EXTERN);
      externSmId.setProp(ACCgpuDecompiler.GPU_STRAGE_SHARED, true);

      smOffsetId = Ident.Local("_ACC_sm_offset", Xtype.intType);
      smOffsetId.setProp(ACCgpuDecompiler.GPU_STRAGE_SHARED, true);
    }

    public Xobject getMaxSize() {
      return maxSize;
    }

    public boolean isUsed() {
      return isUsed;
    }

    Cache alloc(Ident id, XobjList subscript) {
      Cache cache = new Cache(id, subscript);
      smStack.push(cache.cacheTotalSize);

      Xobject nowSize = Xcons.IntConstant(0);
      for (Xobject s : smStack) {
        nowSize = Xcons.binaryOp(Xcode.PLUS_EXPR, nowSize, s);
      }
      maxSize = Xcons.List(Xcode.CONDITIONAL_EXPR, Xcons.binaryOp(Xcode.LOG_LT_EXPR, nowSize, maxSize), Xcons.List(maxSize, nowSize));

      isUsed = true;
      return cache;
    }

// --Commented out by Inspection START (2015/02/24 21:12):
//    void free() {
//      smStack.pop();
//    }
// --Commented out by Inspection STOP (2015/02/24 21:12)

    Block makeInitFunc() {
      return ACCutil.createFuncCallBlock("_ACC_gpu_init_sm_offset", Xcons.List(smOffsetId.getAddr()));
    }
  }

  class Cache {
    final XobjList localIds = Xcons.IDList();
    final Ident cacheSizeArrayId;
    final Ident cacheOffsetArrayId;
    final Xtype elementType;
    final Ident cacheId;
    final Ident varId;
    final XobjList subscripts;
    final int cacheDim;
    Xobject cacheTotalSize;
    final Block initFunc;
    final Block loadBlock;

    Cache(Ident varId, XobjList subscripts) {
      this.varId = varId;
      this.subscripts = subscripts;
      elementType = (varId.Type().isArray()) ? (varId.Type().getArrayElementType()) : varId.Type();
      Xtype cacheType = (varId.Type().isArray()) ? Xtype.Pointer(elementType) : elementType;

      cacheId = Ident.Local(ACC_CACHE_VAR_PREFIX + varId.getName(), cacheType);
      cacheDim = subscripts.Nargs();

      cacheSizeArrayId = Ident.Local("_ACC_cache_size_" + varId.getName(), Xtype.Array(Xtype.intType, cacheDim));
      cacheSizeArrayId.setProp(ACCgpuDecompiler.GPU_STRAGE_SHARED, true);

      cacheOffsetArrayId = Ident.Local("_ACC_cache_offset_" + varId.getName(), Xtype.Array(Xtype.intType, cacheDim));

      localIds.add(cacheOffsetArrayId);
      localIds.add(cacheSizeArrayId);

      initFunc = makeCacheInitializeFunc();
      loadBlock = makeCacheLoadBlock();
    }

    Block makeCacheInitializeFunc() {
      //XobjList getCacheFuncArgs = Xcons.List(_externSm.Ref(), _smOffset.Ref(), cacheId.getAddr(), cacheSizeArrayId.getAddr());//, step, cacheLength);
      XobjList getCacheFuncArgs = Xcons.List(sharedMemory.externSmId.Ref(), sharedMemory.smOffsetId.Ref(), cacheId.getAddr(), cacheSizeArrayId.getAddr());//, step, cacheLength);

      for (Xobject s : subscripts) {
        XobjList simpleSubarray = getSimpleSubarray(s);
        Xobject cacheIdx = simpleSubarray.getArg(0);
        Xobject cacheConstOffset = simpleSubarray.getArg(1);
        Xobject cacheLength = simpleSubarray.getArg(2);

        //find loop
        Loop loop = null;
        for (Loop tmpLoop : loopStack) {
          if (tmpLoop.forBlock.getInductionVar().getName().equals(cacheIdx.getName())) {
            loop = tmpLoop;
            break;
          }
        }
        if (loop == null) ACC.fatal(cacheIdx.getName() + " is not loop variable");

        Xobject step = loop.forBlock.getStep(); //もしcacheがloop変数非依存なら？
        getCacheFuncArgs.mergeList(Xcons.List(step, cacheLength));

      }

      return ACCutil.createFuncCallBlock("_ACC_gpu_init_cache", getCacheFuncArgs);
    }

    Block makeCacheLoadBlock() {
      BlockList cacheLoadBody = Bcons.emptyBody();
      XobjList cacheLoadBodyIds = Xcons.IDList();

      Ident cacheLoadSizeArrayId = Ident.Local("_ACC_cache_load_size_" + varId.getName(), Xtype.Array(Xtype.intType, cacheDim));
      cacheLoadSizeArrayId.setProp(ACCgpuDecompiler.GPU_STRAGE_SHARED, true);
      cacheLoadBodyIds.add(cacheLoadSizeArrayId);


      int dim = 0;
      Xobject totalCacheSize = Xcons.IntConstant(1);

      for (Xobject s : subscripts) {
        XobjList simpleSubarray = getSimpleSubarray(s);
        Xobject cacheIdx = simpleSubarray.getArg(0);
        Xobject cacheConstOffset = simpleSubarray.getArg(1);
        Xobject cacheLength = simpleSubarray.getArg(2);

        Xobject cacheLoadSizeRef = Xcons.arrayRef(Xtype.intType, cacheLoadSizeArrayId.getAddr(), Xcons.List(Xcons.IntConstant(dim)));
        Xobject cacheOffsetRef = Xcons.arrayRef(Xtype.intType, cacheOffsetArrayId.getAddr(), Xcons.List(Xcons.IntConstant(dim)));

        XobjList getLoadSizeFuncArgs = Xcons.List(Xcons.AddrOf(cacheLoadSizeRef), Xcons.arrayRef(Xtype.intType, cacheSizeArrayId.getAddr(), Xcons.List(Xcons.IntConstant(dim))));
        XobjList getOffsetFuncArgs = Xcons.List(Xcons.AddrOf(cacheOffsetRef), cacheIdx, cacheConstOffset);

        //find loop
        Loop loop = null;
        for (Loop tmpLoop : loopStack) {
          if (tmpLoop.forBlock.getInductionVar().getName().equals(cacheIdx.getName())) {
            loop = tmpLoop;
            break;
          }
        }
        if (loop == null) ACC.fatal(cacheIdx.getName() + " is not loop variable");

        Xobject calculatedCacheSize = null;
        if (loop.isParallelized) {
          Xobject abstIdx = loop.abstIdx.Ref();
          Xobject abstCond = loop.abstCond.Ref();
          Xobject abstStep = loop.abstStep.Ref();
          Xobject concStep = loop.forBlock.getStep();
          getLoadSizeFuncArgs.mergeList(Xcons.List(abstIdx, abstCond, abstStep, concStep));

          String methodName = gpuManager.getMethodName(loop.forBlock);
          Xobject blockSize = null;
          if (methodName.endsWith("thread_x")) {
            blockSize = Xcons.Symbol(Xcode.VAR, Xtype.intType, "_ACC_GPU_DIM3_thread_x");
          } else if (methodName.endsWith("thread_y")) {
            blockSize = Xcons.Symbol(Xcode.VAR, Xtype.intType, "_ACC_GPU_DIM3_thread_y");
          } else if (methodName.endsWith("thread_z")) {
            blockSize = Xcons.Symbol(Xcode.VAR, Xtype.intType, "_ACC_GPU_DIM3_thread_z");
          } else {
            blockSize = Xcons.IntConstant(1);
          }
          calculatedCacheSize = Xcons.binaryOp(Xcode.PLUS_EXPR, Xcons.binaryOp(Xcode.MUL_EXPR, Xcons.binaryOp(Xcode.MINUS_EXPR, blockSize, Xcons.IntConstant(1)), concStep), cacheLength);
        } else {
          Xobject zeroObj = Xcons.IntConstant(0);
          Xobject concStep = loop.forBlock.getStep();
          getLoadSizeFuncArgs.mergeList(Xcons.List(zeroObj, zeroObj, zeroObj, concStep));
          calculatedCacheSize = cacheLength;
        }

        Block getLoadSizeFuncCall = ACCutil.createFuncCallBlock("_ACC_gpu_get_cache_load_size", getLoadSizeFuncArgs);
        Block getOffsetFuncCall = ACCutil.createFuncCallBlock("_ACC_gpu_get_cache_offset", getOffsetFuncArgs);
        cacheLoadBody.add(getLoadSizeFuncCall);
        cacheLoadBody.add(getOffsetFuncCall);

        totalCacheSize = Xcons.binaryOp(Xcode.MUL_EXPR, totalCacheSize, calculatedCacheSize);
        dim++;
      }
      //cacheSizeList.add(Xcons.binaryOp(Xcode.MUL_EXPR, totalCacheSize, Xcons.SizeOf(elementType)));
      cacheTotalSize = Xcons.binaryOp(Xcode.MUL_EXPR, totalCacheSize, Xcons.SizeOf(elementType));

      //make load for loop
      Block dummyInnerMostBlock = Bcons.emptyBlock(); //dummy block
      Block loadLoopBlock = null;//dummyInnerMostBlock;
      XobjList lhsArrayRefList = Xcons.List();
      XobjList rhsArrayRefList = Xcons.List();
      for (int d = 0; d < cacheDim; d++) { //from higher dim
        Ident tmpIter = Ident.Local("_ACC_iter_idx" + d, Xtype.intType);
        cacheLoadBodyIds.add(tmpIter);
        Xobject tmpIterInit, tmpIterCond, tmpIterStep;
        tmpIterCond = Xcons.arrayRef(Xtype.intType, cacheLoadSizeArrayId.getAddr(), Xcons.List(Xcons.IntConstant(d)));
        if (d == cacheDim - 1) {
          Ident thread_x_id = Ident.Local("_ACC_thread_x_id", Xtype.intType); //this is macro
          tmpIterInit = thread_x_id.Ref();
          Ident block_size_x = Ident.Local("_ACC_block_size_x", Xtype.intType); //this is macro
          tmpIterStep = block_size_x.Ref();
        } else if (d == cacheDim - 2) {
          Ident thread_y_id = Ident.Local("_ACC_thread_y_id", Xtype.intType); //this is macro
          tmpIterInit = thread_y_id.Ref();
          Ident block_size_y = Ident.Local("_ACC_block_size_y", Xtype.intType); //this is macro
          tmpIterStep = block_size_y.Ref();
        } else if (d == cacheDim - 3) {
          Ident thread_z_id = Ident.Local("_ACC_thread_z_id", Xtype.intType); //this is macro
          tmpIterInit = thread_z_id.Ref();
          Ident block_size_z = Ident.Local("_ACC_block_size_z", Xtype.intType); //this is macro
          tmpIterStep = block_size_z.Ref();
        } else {
          tmpIterInit = Xcons.IntConstant(0);
          tmpIterStep = Xcons.IntConstant(1);
        }
        if (lhsArrayRefList.isEmpty()) {
          lhsArrayRefList.add(tmpIter.Ref());
        } else {
          Xobject newRef = lhsArrayRefList.getArg(0);
          newRef = Xcons.binaryOp(Xcode.PLUS_EXPR, Xcons.binaryOp(Xcode.MUL_EXPR, newRef, Xcons.arrayRef(Xtype.intType, cacheSizeArrayId.getAddr(), Xcons.List(Xcons.IntConstant(d)))), tmpIter.Ref());
          lhsArrayRefList.setArg(0, newRef);
        }
        rhsArrayRefList.add(Xcons.binaryOp(Xcode.MINUS_EXPR, tmpIter.Ref(), Xcons.arrayRef(Xtype.intType, cacheOffsetArrayId.getAddr(), Xcons.List(Xcons.IntConstant(d)))));
        if (loadLoopBlock == null) {
          loadLoopBlock = Bcons.FORall(tmpIter.Ref(), tmpIterInit, tmpIterCond, tmpIterStep, Xcode.LOG_LT_EXPR, Bcons.blockList(dummyInnerMostBlock));
        } else {
          Block newDummyInnerMostBlock = Bcons.emptyBlock();
          Block replaceBlock = Bcons.FORall(tmpIter.Ref(), tmpIterInit, tmpIterCond, tmpIterStep, Xcode.LOG_LT_EXPR, Bcons.blockList(newDummyInnerMostBlock));
          dummyInnerMostBlock.replace(replaceBlock);
          dummyInnerMostBlock = newDummyInnerMostBlock;
        }

        //loadLoopBlock = Bcons.FORall(tmpIter.Ref(), tmpIterInit, tmpIterCond, tmpIterStep, Xcode.LOG_LT_EXPR, Bcons.blockList(loadLoopBlock));
      }
      Xobject innerMostObject = Xcons.Set(Xcons.arrayRef(elementType, cacheId.getAddr(), lhsArrayRefList), Xcons.arrayRef(elementType, varId.getAddr(), rhsArrayRefList));
      dummyInnerMostBlock.replace(Bcons.Statement(innerMostObject));

      cacheLoadBody.add(loadLoopBlock);
      cacheLoadBody.add(ACCutil.createFuncCallBlock("_ACC_gpu_barrier", Xcons.List()));
      cacheLoadBody.setIdentList(cacheLoadBodyIds);
      //cacheLoadBody.setDecls(ACCutil.getDecls(cacheLoadBodyIds));
      cacheLoadBody.setIdentList(cacheLoadBodyIds);
      return Bcons.COMPOUND(cacheLoadBody);
    }

    void rewrite(Block b) {
      BasicBlockExprIterator iter = new BasicBlockExprIterator(b);
      for (iter.init(); !iter.end(); iter.next()) {
        Xobject expr = iter.getExpr();
        topdownXobjectIterator exprIter = new topdownXobjectIterator(expr);
        for (exprIter.init(); !exprIter.end(); exprIter.next()) {
          Xobject x = exprIter.getXobject();
          switch (x.Opcode()) {
          case VAR: {
            String varName = x.getName();
            if(varName.equals(varId.getName())){
              exprIter.setXobject(cacheId.Ref());
            }
          }
          break;
          case ARRAY_REF: {
            String arrayName = x.getArg(0).getName();
            if(! arrayName.equals(varId.getName())) break;
            XobjList arrayIdxList = (XobjList) x.getArg(1);
            Xobject arrayIdx = null;

            int dim = 0;
            for (Xobject idx : arrayIdxList) {
              //newArrayIdxList.add(Xcons.binaryOp(Xcode.PLUS_EXPR, idx, offsetId.Ref()));
              Xobject newArrayIdx = Xcons.binaryOp(Xcode.PLUS_EXPR, idx, Xcons.arrayRef(Xtype.intType, cacheOffsetArrayId.getAddr(), Xcons.List(Xcons.IntConstant(dim))));
              if (arrayIdx == null) {
                arrayIdx = newArrayIdx;
              } else {
                arrayIdx = Xcons.binaryOp(Xcode.PLUS_EXPR, Xcons.binaryOp(Xcode.MUL_EXPR, arrayIdx, Xcons.arrayRef(Xtype.intType, cacheSizeArrayId.getAddr(), Xcons.List(Xcons.IntConstant(dim)))), newArrayIdx);
              }
              dim++;
            }
            Xobject newObj = Xcons.arrayRef(x.Type(), cacheId.Ref(), Xcons.List(arrayIdx));
            exprIter.setXobject(newObj);
          }
          break;
          }
        }
      }
    }

    private XobjList getSimpleSubarray(Xobject s) {
      Xobject loopIdx;
      Xobject constOffset;
      Xobject length;

      Xobject lower;
      if (s.Opcode() != Xcode.LIST) {
        lower = s;
        length = Xcons.IntConstant(1);
      } else {
        lower = s.getArg(0);
        length = s.getArgOrNull(1);
      }

      switch (lower.Opcode()) {
      case PLUS_EXPR:
        loopIdx = lower.getArg(0);
        constOffset = lower.getArg(1);
        break;
      case MINUS_EXPR:
        loopIdx = lower.getArg(0);
        constOffset = Xcons.unaryOp(Xcode.UNARY_MINUS_EXPR, lower.getArg(1));
        break;
      case INT_CONSTANT:
        loopIdx = Xcons.IntConstant(0);
        constOffset = lower;
        break;
      default:
        loopIdx = lower;
        constOffset = Xcons.IntConstant(0);
      }

      return Xcons.List(loopIdx, constOffset, length);
    }
  }

  class ReductionManager {
    Ident counterPtr = null;
    Ident tempPtr = null;
    final List<Reduction> reductionList = new ArrayList<Reduction>();
    Xobject totalElementSize = Xcons.IntConstant(0);
    final Map<Reduction, Xobject> offsetMap = new HashMap<Reduction, Xobject>();
    Ident isLastVar = null;

    ReductionManager() {
      counterPtr = Ident.Param(ACC_REDUCTION_CNT_VAR, Xtype.Pointer(Xtype.unsignedType));//Ident.Var("_ACC_GPU_RED_CNT", Xtype.unsignedType, Xtype.Pointer(Xtype.unsignedType), VarScope.GLOBAL);
      tempPtr = Ident.Param(ACC_REDUCTION_TMP_VAR, Xtype.voidPtrType);//Ident.Var("_ACC_GPU_RED_TMP", Xtype.voidPtrType, Xtype.Pointer(Xtype.voidPtrType), VarScope.GLOBAL);
      isLastVar = Ident.Local("_ACC_GPU_IS_LAST_BLOCK", Xtype.intType);
      isLastVar.setProp(ACCgpuDecompiler.GPU_STRAGE_SHARED, true);
    }

    public XobjectDef makeReductionKernelDef(String deviceKernelName) {
      BlockList reductionKernelBody = Bcons.emptyBody();

      XobjList deviceKernelParamIds = Xcons.IDList();
      Xobject blockIdx = Xcons.Symbol(Xcode.VAR, Xtype.intType, "_ACC_block_x_id");
      Ident numBlocksId = Ident.Param("_ACC_GPU_RED_NUM", Xtype.intType);
      int count = 0;
      Iterator<Reduction> blockRedIter = reductionManager.BlockReductionIterator();
      while (blockRedIter.hasNext()) {
        Reduction reduction = blockRedIter.next();
        if (!(reduction.useBlock() && reduction.usesTmp())) continue;

        Block blockReduction = reduction.makeBlockReductionFuncCall(tempPtr, offsetMap.get(reduction), numBlocksId);//reduction.makeBlockReductionFuncCall(tempPtr, tmpOffsetElementSize)
        Block ifBlock = Bcons.IF(Xcons.binaryOp(Xcode.LOG_EQ_EXPR, blockIdx, Xcons.IntConstant(count)), blockReduction, null);
        reductionKernelBody.add(ifBlock);
        count++;
      }

      for (Xobject x : _outerIdList) {
        Ident id = (Ident) x;
        Reduction reduction = reductionManager.findReduction(id);
        if (reduction != null && reduction.usesTmp()) {
          deviceKernelParamIds.add(makeParamId_new(id)); //getVarId();
        }
      }

      deviceKernelParamIds.add(tempPtr);
      deviceKernelParamIds.add(numBlocksId);

      Ident deviceKernelId = kernelInfo.getGlobalDecl().getEnvDevice().declGlobalIdent(deviceKernelName, Xtype.Function(Xtype.voidType));
      ((FunctionType) deviceKernelId.Type()).setFuncParamIdList(deviceKernelParamIds);
      return XobjectDef.Func(deviceKernelId, deviceKernelParamIds, null, Bcons.COMPOUND(reductionKernelBody).toXobject());
    }

    public XobjList getBlockReductionParamIds() {
      return Xcons.List(Xcode.ID_LIST, tempPtr, counterPtr);
    }

    public Block makeLocalVarInitFuncs() {
      BlockList body = Bcons.emptyBody();
      Iterator<Reduction> blockRedIter = reductionManager.BlockReductionIterator();
      while (blockRedIter.hasNext()) {
        Reduction reduction = blockRedIter.next();
        body.add(reduction.makeInitReductionVarFuncCall());
      }

      if (body.isSingle()) {
        return body.getHead();
      } else {
        return Bcons.COMPOUND(body);
      }
    }

    public XobjList getBlockReductionLocalIds() {
      XobjList blockLocalIds = Xcons.IDList();
      Iterator<Reduction> blockRedIter = reductionManager.BlockReductionIterator();
      while (blockRedIter.hasNext()) {
        Reduction reduction = blockRedIter.next();
        blockLocalIds.add(reduction.getLocalReductionVarId());
      }
      return blockLocalIds;
    }

    public Block makeReduceAndFinalizeFuncs() {
      /*
       * {
       *   __shared__ int _ACC_GPU_IS_LAST_BLOCK;
       *   
       *   _ACC_gpu_reduction_thread(...);
       *   
       *   _ACC_gpu_is_last_block(&_ACC_GPU_IS_LAST,_ACC_GPU_RED_CNT);
       *   if((_ACC_GPU_IS_LAST)!=(0)){
       *     _ACC_gpu_reduction_block(...);
       *   }
       * }
       */
      BlockList body = Bcons.emptyBody();
      BlockList thenBody = Bcons.emptyBody();
      BlockList tempWriteBody = Bcons.emptyBody();

      // add funcs
      Iterator<Reduction> blockRedIter = reductionManager.BlockReductionIterator();
      while (blockRedIter.hasNext()) {
        Reduction reduction = blockRedIter.next();
        Ident tmpVar = Ident.Local("_ACC_gpu_reduction_tmp_" + reduction.var.getName(), reduction.varId.Type());
        if (reduction.useThread()) {
          body.addIdent(tmpVar);
          body.add(ACCutil.createFuncCallBlock("_ACC_gpu_init_reduction_var", Xcons.List(tmpVar.getAddr(), Xcons.IntConstant(reduction.getReductionKindInt()))));
          body.add(reduction.makeThreadReductionFuncCall(tmpVar));
        }
        if (reduction.useBlock() && reduction.usesTmp()) {
          if (reduction.useThread()) {
            tempWriteBody.add(reduction.makeTempWriteFuncCall(tmpVar, tempPtr, offsetMap.get(reduction)));
            thenBody.add(reduction.makeSingleBlockReductionFuncCall(tmpVar));
          } else {
            tempWriteBody.add(reduction.makeTempWriteFuncCall(tempPtr, offsetMap.get(reduction)));
            thenBody.add(reduction.makeSingleBlockReductionFuncCall());
          }
        } else {
          if (reduction.useThread()) {
            body.add(reduction.makeAtomicBlockReductionFuncCall(tmpVar));
          } else {
            body.add(reduction.makeAtomicBlockReductionFuncCall(null));
          }
        }
      }

      if (!thenBody.isEmpty()) {
        Xobject grid_dim = Xcons.Symbol(Xcode.VAR, Xtype.unsignedType, "_ACC_grid_x_dim");
        body.add(Bcons.IF(Xcons.binaryOp(Xcode.LOG_EQ_EXPR, grid_dim, Xcons.IntConstant(1)), Bcons.COMPOUND(thenBody), Bcons.COMPOUND(tempWriteBody)));
      }

      return Bcons.COMPOUND(body);
    }

    Reduction addReduction(ACCvar var, EnumSet<ACCpragma> execMethodSet) {
      Reduction reduction = new Reduction(var, execMethodSet);
      reductionList.add(reduction);

      if(! execMethodSet.contains(ACCpragma.GANG)) return reduction;

      if (!reduction.usesTmp()) return reduction;

      //tmp setting
      offsetMap.put(reduction, totalElementSize);


      Xtype varType = var.getId().Type();
      Xobject elementSize;
      if (varType.isPointer()) {
        elementSize = Xcons.SizeOf(varType.getRef());
      } else {
        elementSize = Xcons.SizeOf(varType);
      }
      totalElementSize = Xcons.binaryOp(Xcode.PLUS_EXPR, totalElementSize, elementSize);
      return reduction;
    }

    Reduction findReduction(Ident id) {
      for (Reduction red : reductionList) {
        if (red.varId == id) {
          return red;
        }
      }
      return null;
    }

    Iterator<Reduction> BlockReductionIterator() {
      return new BlockReductionIterator(reductionList);
    }

    boolean hasUsingTmpReduction() {
      return !offsetMap.isEmpty();
    }

    class BlockReductionIterator implements Iterator<Reduction> {
      final Iterator<Reduction> reductionIterator;
      Reduction re;

      public BlockReductionIterator(List<Reduction> reductionList) {
        this.reductionIterator = reductionList.iterator();
      }

      @Override
      public boolean hasNext() {
        while (true) {
          if (reductionIterator.hasNext()) {
            re = reductionIterator.next();
            if (re.useBlock()) {
              return true;
            }
          } else {
            return false;
          }
        }
      }

      @Override
      public Reduction next() {
        return re;
      }

      @Override
      public void remove() {
        //do nothing
      }
    }
  }

  class Reduction {
    final EnumSet<ACCpragma> execMethodSet;  //final ACCpragma execMethod;
    final Ident localVarId;
    final Ident varId;
    // --Commented out by Inspection (2015/02/24 21:12):Ident launchFuncLocalId;
    final ACCvar var;

    //Ident tmpId;
    Reduction(ACCvar var, EnumSet<ACCpragma> execMethodSet) {
      this.var = var;
      this.varId = var.getId();
      this.execMethodSet = EnumSet.copyOf(execMethodSet); //execMethod;

      //generate local var id
      String reductionVarPrefix = ACC_REDUCTION_VAR_PREFIX;

      if(execMethodSet.contains(ACCpragma.GANG)) reductionVarPrefix += "b";
      if(execMethodSet.contains(ACCpragma.VECTOR)) reductionVarPrefix += "t";

      reductionVarPrefix += "_";

      localVarId = Ident.Local(reductionVarPrefix + varId.getName(), varId.Type());
      if (execMethodSet.contains(ACCpragma.GANG) && ! execMethodSet.contains(ACCpragma.VECTOR)){ //execMethod == ACCpragma._BLOCK) {
        localVarId.setProp(ACCgpuDecompiler.GPU_STRAGE_SHARED, true);
      }
    }

    public Block makeSingleBlockReductionFuncCall(Ident tmpPtrId) {
      XobjList args = Xcons.List(varId.Ref(), tmpPtrId.Ref(), Xcons.IntConstant(getReductionKindInt()));
      return ACCutil.createFuncCallBlock("_ACC_gpu_reduction_singleblock", args);
    }

    public Block makeSingleBlockReductionFuncCall() {
      XobjList args = Xcons.List(varId.Ref(), localVarId.Ref(), Xcons.IntConstant(getReductionKindInt()));
      return ACCutil.createFuncCallBlock("_ACC_gpu_reduction_singleblock", args);
    }

    public void rewrite(Block b) {
      BasicBlockExprIterator iter = new BasicBlockExprIterator(b);
      for (iter.init(); !iter.end(); iter.next()) {
        Xobject expr = iter.getExpr();
        topdownXobjectIterator exprIter = new topdownXobjectIterator(expr);
        for (exprIter.init(); !exprIter.end(); exprIter.next()) {
          Xobject x = exprIter.getXobject();
          switch (x.Opcode()) {
          case VAR: {
            String varName = x.getName();
            if (varName.equals(varId.getName())) {
              exprIter.setXobject(localVarId.Ref());
            }
          }
          break;
          case VAR_ADDR: {
            String varName = x.getName();
            if (varName.equals(varId.getName())) {
              exprIter.setXobject(localVarId.getAddr());
            }
          }
          break;
          }
        }
      }
    }

    public boolean useThread() {
      return execMethodSet.contains(ACCpragma.VECTOR); //execMethod != ACCpragma._BLOCK;
    }

    public Ident getLocalReductionVarId() {
      return localVarId;
    }

    public Block makeInitReductionVarFuncCall() {
      int reductionKind = getReductionKindInt();

      if (!execMethodSet.contains(ACCpragma.VECTOR)){ //execMethod == ACCpragma._BLOCK) {
        return ACCutil.createFuncCallBlock("_ACC_gpu_init_reduction_var_single", Xcons.List(localVarId.getAddr(), Xcons.IntConstant(reductionKind)));
      } else {
        return ACCutil.createFuncCallBlock("_ACC_gpu_init_reduction_var", Xcons.List(localVarId.getAddr(), Xcons.IntConstant(reductionKind)));
      }
    }

    public Block makeBlockReductionFuncCall(Ident tmpPtrId, Xobject tmpOffsetElementSize, Ident numBlocks) {
      XobjList args = Xcons.List(varId.Ref(), Xcons.IntConstant(getReductionKindInt()), tmpPtrId.Ref(), tmpOffsetElementSize);
      if (numBlocks != null) {
        args.add(numBlocks.Ref());
      }
      return ACCutil.createFuncCallBlock("_ACC_gpu_reduction_block", args);
    }

    public Block makeAtomicBlockReductionFuncCall(Ident tmpVar) {
      XobjList args;
      if (tmpVar != null) {
        args = Xcons.List(varId.Ref(), Xcons.IntConstant(getReductionKindInt()), tmpVar.Ref());
      } else {
        args = Xcons.List(varId.Ref(), Xcons.IntConstant(getReductionKindInt()), localVarId.Ref());
      }
      return ACCutil.createFuncCallBlock("_ACC_gpu_reduction_block", args);
    }

    public Block makeThreadReductionFuncCall() {
      //XobjList args = Xcons.List(localVarId.Ref(), Xcons.IntConstant(getReductionKindInt()));
      if (! execMethodSet.contains(ACCpragma.GANG)){ //execMethod == ACCpragma._THREAD) {
        //args.cons(varId.getAddr());
        return makeThreadReductionFuncCall(varId);
      } else {
        return makeThreadReductionFuncCall(localVarId);
        //args.cons(localVarId.getAddr());
      }
      //return ACCutil.createFuncCallBlock("_ACC_gpu_reduction_thread", args);
    }

    public Block makeThreadReductionFuncCall(Ident varId) {
      XobjList args = Xcons.List(varId.getAddr(), localVarId.Ref(), Xcons.IntConstant(getReductionKindInt()));
      return ACCutil.createFuncCallBlock("_ACC_gpu_reduction_thread", args);
    }

    public Block makeTempWriteFuncCall(Ident tmpPtrId, Xobject tmpOffsetElementSize) {
      return ACCutil.createFuncCallBlock("_ACC_gpu_reduction_tmp", Xcons.List(localVarId.Ref(), tmpPtrId.Ref(), tmpOffsetElementSize));
    }

    public Block makeTempWriteFuncCall(Ident id, Ident tmpPtrId, Xobject tmpOffsetElementSize) {
      return ACCutil.createFuncCallBlock("_ACC_gpu_reduction_tmp", Xcons.List(id.Ref(), tmpPtrId.Ref(), tmpOffsetElementSize));
    }

    private int getReductionKindInt() {
      ACCpragma pragma = var.getReductionOperator();
      if (!pragma.isReduction()) ACC.fatal(pragma.getName() + " is not reduction clause");
      switch (pragma) {
      case REDUCTION_PLUS:
        return 0;
      case REDUCTION_MUL:
        return 1;
      case REDUCTION_MAX:
        return 2;
      case REDUCTION_MIN:
        return 3;
      case REDUCTION_BITAND:
        return 4;
      case REDUCTION_BITOR:
        return 5;
      case REDUCTION_BITXOR:
        return 6;
      case REDUCTION_LOGAND:
        return 7;
      case REDUCTION_LOGOR:
        return 8;
      default:
        return -1;
      }
    }

    public boolean useBlock() {
      return execMethodSet.contains(ACCpragma.GANG);
    }

    public boolean usesTmp() {
      ACCpragma op = var.getReductionOperator();
      switch (var.getId().Type().getBasicType()) {
      case BasicType.FLOAT:
      case BasicType.INT:
        return op == ACCpragma.REDUCTION_MUL;
      }
      return true;
    }
  }

  private class BlockListBuilder{
    private final List<Block> initBlockList = new ArrayList<Block>();
    private final List<Block> finalizeBlockList = new ArrayList<Block>();
    private final List<Block> mainBlockList = new ArrayList<Block>();
    private final BlockList blockList = Bcons.emptyBody();
    public void addInitBlock(Block b) { initBlockList.add(b); }
    public void addFinalizeBlock(Block b) { finalizeBlockList.add(b); }
    public void add(Block b) { mainBlockList.add(b); }
    public Ident declLocalIdent(String name, Xtype type) { return blockList.declLocalIdent(name,type); }
    public Ident declLocalIdent(String name, Xtype type, Xobject init) { return blockList.declLocalIdent(name, type, StorageClass.AUTO, init); }
    public void addIdent(Ident id) { blockList.addIdent(id); }

    public BlockList build(){
      BlockList body = blockList.copy();
      for(Block b : initBlockList) body.add(b);
      for(Block b : mainBlockList) body.add(b);
      for(Block b : finalizeBlockList) body.add(b);
      return body;
    }
  }
}

#ifdef UNITTEST_DZBRIDGE

#include "UnitTest_DzUnityAction.h"
#include "DzUnityAction.h"
#include "OpenFBXInterface.h"

namespace
{
	FbxNode* createTestBone(FbxScene* Scene, const char* Name, FbxNode* Parent)
	{
		FbxNode* Node = FbxNode::Create(Scene, Name);
		FbxSkeleton* Skeleton = FbxSkeleton::Create(Scene, Name);
		Skeleton->SetSkeletonType(FbxSkeleton::eLimbNode);
		Node->SetNodeAttribute(Skeleton);
		Parent->AddChild(Node);
		return Node;
	}

	FbxNode* createTestMesh(FbxScene* Scene, const char* Name)
	{
		FbxNode* Node = FbxNode::Create(Scene, Name);
		FbxMesh* Mesh = FbxMesh::Create(Scene, Name);
		Mesh->InitControlPoints(1);
		Mesh->SetControlPointAt(FbxVector4(0.0, 0.0, 0.0), 0);
		Node->SetNodeAttribute(Mesh);
		Scene->GetRootNode()->AddChild(Node);
		return Node;
	}

	FbxCluster* addTestCluster(
		FbxScene* Scene,
		FbxNode* MeshNode,
		FbxNode* Bone,
		const FbxAMatrix& MeshMatrix,
		const FbxAMatrix& BindMatrix,
		FbxCluster::ELinkMode LinkMode = FbxCluster::eNormalize)
	{
		FbxSkin* Skin = FbxSkin::Create(Scene, "TestSkin");
		FbxCluster* Cluster = FbxCluster::Create(Scene, "TestCluster");
		Cluster->SetLink(Bone);
		Cluster->SetLinkMode(LinkMode);
		Cluster->SetTransformMatrix(MeshMatrix);
		Cluster->SetTransformLinkMatrix(BindMatrix);
		Cluster->AddControlPointIndex(0, 0.75);
		Skin->AddCluster(Cluster);
		MeshNode->GetMesh()->AddDeformer(Skin);
		return Cluster;
	}

	FbxAMatrix translatedMatrix(double X, double Y, double Z)
	{
		FbxAMatrix Matrix;
		Matrix.SetIdentity();
		Matrix.SetT(FbxVector4(X, Y, Z));
		return Matrix;
	}

	bool testMatricesMatch(const FbxAMatrix& Left, const FbxAMatrix& Right)
	{
		for (int Row = 0; Row < 4; ++Row)
		{
			for (int Column = 0; Column < 4; ++Column)
			{
				if (qAbs(Left.Get(Row, Column) - Right.Get(Row, Column)) > 0.000001)
					return false;
			}
		}
		return true;
	}

	int countTestBones(FbxNode* Node, const QString& Name)
	{
		if (Node == nullptr)
			return 0;
		FbxNodeAttribute* Attribute = Node->GetNodeAttribute();
		int Count = Attribute != nullptr && Attribute->GetAttributeType() == FbxNodeAttribute::eSkeleton &&
			QString::fromUtf8(Node->GetName()) == Name ? 1 : 0;
		for (int ChildIndex = 0; ChildIndex < Node->GetChildCount(); ++ChildIndex)
			Count += countTestBones(Node->GetChild(ChildIndex), Name);
		return Count;
	}
}


UnitTest_DzUnityAction::UnitTest_DzUnityAction()
{
	m_testObject = (QObject*) new DzUnityForkAction();
}

bool UnitTest_DzUnityAction::runUnitTests()
{
	RUNTEST(_DzBridgeUnityAction);
	RUNTEST(setInstallUnityFiles);
	RUNTEST(getInstallUnityFiles);
	RUNTEST(setExportStrandHairAlembic);
	RUNTEST(getExportStrandHairAlembic);
	RUNTEST(setExportStrandHairBlenderAlembic);
	RUNTEST(getExportStrandHairBlenderAlembic);
	RUNTEST(setStrandHairBlenderExecutable);
	RUNTEST(getStrandHairBlenderExecutable);
	RUNTEST(setRunStrandHairBlenderBake);
	RUNTEST(getRunStrandHairBlenderBake);
	RUNTEST(executeAction);
	RUNTEST(createUI);
	RUNTEST(writeConfiguration);
	RUNTEST(setExportOptions);
	RUNTEST(createUnityFiles);
	RUNTEST(readGuiRootFolder);
	RUNTEST(consolidateConnectedFollowerRig);
	RUNTEST(preserveFollowerRigWithUniqueChild);
	RUNTEST(preserveUnsupportedFollowerCluster);
	RUNTEST(preserveAmbiguousCanonicalRig);
	RUNTEST(preserveInconsistentCanonicalBindRig);

	return true;
}

bool UnitTest_DzUnityAction::_DzBridgeUnityAction(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(new DzUnityForkAction());
	return bResult;
}

bool UnitTest_DzUnityAction::setInstallUnityFiles(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->setInstallUnityFiles(false));
	return bResult;
}

bool UnitTest_DzUnityAction::getInstallUnityFiles(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->getInstallUnityFiles());
	return bResult;
}

bool UnitTest_DzUnityAction::setExportStrandHairAlembic(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->setExportStrandHairAlembic(false));
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->setExportStrandHairAlembic(true));
	return bResult;
}

bool UnitTest_DzUnityAction::getExportStrandHairAlembic(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->getExportStrandHairAlembic());
	return bResult;
}

bool UnitTest_DzUnityAction::setExportStrandHairBlenderAlembic(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->setExportStrandHairBlenderAlembic(false));
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->setExportStrandHairBlenderAlembic(true));
	return bResult;
}

bool UnitTest_DzUnityAction::getExportStrandHairBlenderAlembic(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->getExportStrandHairBlenderAlembic());
	return bResult;
}

bool UnitTest_DzUnityAction::setStrandHairBlenderExecutable(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->setStrandHairBlenderExecutable("C:/Program Files/Blender Foundation/Blender 4.4/blender.exe"));
	return bResult;
}

bool UnitTest_DzUnityAction::getStrandHairBlenderExecutable(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->getStrandHairBlenderExecutable());
	return bResult;
}

bool UnitTest_DzUnityAction::setRunStrandHairBlenderBake(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->setRunStrandHairBlenderBake(false));
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->setRunStrandHairBlenderBake(true));
	return bResult;
}

bool UnitTest_DzUnityAction::getRunStrandHairBlenderBake(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->getRunStrandHairBlenderBake());
	return bResult;
}

bool UnitTest_DzUnityAction::executeAction(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->executeAction());
	return bResult;
}

bool UnitTest_DzUnityAction::createUI(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->createUI());
	return bResult;
}

bool UnitTest_DzUnityAction::writeConfiguration(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->writeConfiguration());
	return bResult;
}

bool UnitTest_DzUnityAction::setExportOptions(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	DzFileIOSettings arg;
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->setExportOptions(arg));
	return bResult;
}

bool UnitTest_DzUnityAction::createUnityFiles(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->createUnityFiles());
	return bResult;
}

bool UnitTest_DzUnityAction::readGuiRootFolder(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->readGuiRootFolder());
	return bResult;
}

bool UnitTest_DzUnityAction::consolidateConnectedFollowerRig(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	FbxScene* Scene = OpenFBXInterface::GetInterface()->CreateScene("ConnectedFollowerRigTest");
	FbxNode* Spine = createTestBone(Scene, "spine4", Scene->GetRootNode());
	FbxNode* MainShoulder = createTestBone(Scene, "l_shoulder", Spine);
	FbxNode* MainUpperArm = createTestBone(Scene, "l_upperarm", MainShoulder);
	FbxNode* FollowerShoulder = createTestBone(Scene, "l_shoulder", Spine);
	FbxNode* FollowerUpperArm = createTestBone(Scene, "l_upperarm", FollowerShoulder);
	FbxNode* MainMesh = createTestMesh(Scene, "Genesis9.Shape");
	FbxNode* HairMesh = createTestMesh(Scene, "Hair.Shape");
	FbxAMatrix MeshMatrix = translatedMatrix(3.0, 4.0, 5.0);
	FbxAMatrix MainShoulderBind = translatedMatrix(1.0, 2.0, 3.0);
	FbxAMatrix MainUpperArmBind = translatedMatrix(4.0, 5.0, 6.0);
	addTestCluster(Scene, MainMesh, MainShoulder, MeshMatrix, MainShoulderBind);
	addTestCluster(Scene, MainMesh, MainUpperArm, MeshMatrix, MainUpperArmBind);
	FbxCluster* HairShoulderCluster = addTestCluster(
		Scene, HairMesh, FollowerShoulder, MeshMatrix, translatedMatrix(10.0, 20.0, 30.0));
	FbxCluster* HairUpperArmCluster = addTestCluster(
		Scene, HairMesh, FollowerUpperArm, MeshMatrix, translatedMatrix(40.0, 50.0, 60.0));
	FbxPose* BindPose = FbxPose::Create(Scene, "TestBindPose");
	BindPose->SetIsBindPose(true);
	BindPose->Add(MainShoulder, MainShoulderBind);
	BindPose->Add(FollowerShoulder, translatedMatrix(10.0, 20.0, 30.0));
	Scene->AddPose(BindPose);

	DzUnityForkAction::FollowerRigConsolidationResult Result =
		qobject_cast<DzUnityForkAction*>(m_testObject)->consolidateFollowerRigs(Scene, QStringList() << "Genesis9.Shape");
	FbxAMatrix ActualShoulderBind;
	FbxAMatrix ActualUpperArmBind;
	FbxAMatrix ActualMeshMatrix;
	HairShoulderCluster->GetTransformLinkMatrix(ActualShoulderBind);
	HairUpperArmCluster->GetTransformLinkMatrix(ActualUpperArmBind);
	HairShoulderCluster->GetTransformMatrix(ActualMeshMatrix);

	if (!Result.Success || !Result.Changed || Result.BonesRemoved != 2 || Result.ClustersRedirected != 2)
	{
		LOGTEST_FAILED("The connected follower component was not consolidated.");
		bResult = false;
	}
	if (HairShoulderCluster->GetLink() != MainShoulder || HairUpperArmCluster->GetLink() != MainUpperArm)
	{
		LOGTEST_FAILED("Follower clusters were not redirected to the canonical bones.");
		bResult = false;
	}
	if (!testMatricesMatch(ActualShoulderBind, MainShoulderBind) ||
		!testMatricesMatch(ActualUpperArmBind, MainUpperArmBind) ||
		!testMatricesMatch(ActualMeshMatrix, MeshMatrix))
	{
		LOGTEST_FAILED("Cluster bind or mesh matrices were not preserved correctly.");
		bResult = false;
	}
	if (HairShoulderCluster->GetLinkMode() != FbxCluster::eNormalize ||
		HairShoulderCluster->GetControlPointIndicesCount() != 1 ||
		HairShoulderCluster->GetControlPointIndices()[0] != 0 ||
		qAbs(HairShoulderCluster->GetControlPointWeights()[0] - 0.75) > 0.000001)
	{
		LOGTEST_FAILED("Follower cluster mode, indices, or weights changed.");
		bResult = false;
	}
	if (countTestBones(Scene->GetRootNode(), "l_shoulder") != 1 ||
		countTestBones(Scene->GetRootNode(), "l_upperarm") != 1)
	{
		LOGTEST_FAILED("Duplicate follower bones remain after consolidation.");
		bResult = false;
	}
	if (BindPose->GetCount() != 1 || BindPose->GetNode(0) != MainShoulder)
	{
		LOGTEST_FAILED("The obsolete follower bone remained in an FBX pose.");
		bResult = false;
	}
	Scene->Destroy();
	return bResult;
}

bool UnitTest_DzUnityAction::preserveFollowerRigWithUniqueChild(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	FbxScene* Scene = OpenFBXInterface::GetInterface()->CreateScene("UniqueChildFollowerRigTest");
	FbxNode* Spine = createTestBone(Scene, "spine3", Scene->GetRootNode());
	FbxNode* MainBone = createTestBone(Scene, "l_pectoral", Spine);
	FbxNode* FollowerBone = createTestBone(Scene, "l_pectoral", Spine);
	createTestBone(Scene, "hair_custom_bone", FollowerBone);
	FbxNode* MainMesh = createTestMesh(Scene, "Genesis9.Shape");
	FbxNode* HairMesh = createTestMesh(Scene, "Hair.Shape");
	addTestCluster(Scene, MainMesh, MainBone, translatedMatrix(0.0, 0.0, 0.0), translatedMatrix(1.0, 0.0, 0.0));
	FbxCluster* HairCluster = addTestCluster(
		Scene, HairMesh, FollowerBone, translatedMatrix(0.0, 0.0, 0.0), translatedMatrix(2.0, 0.0, 0.0));

	DzUnityForkAction::FollowerRigConsolidationResult Result =
		qobject_cast<DzUnityForkAction*>(m_testObject)->consolidateFollowerRigs(Scene, QStringList() << "Genesis9.Shape");
	if (!Result.Success || Result.Changed || Result.Warnings.isEmpty() || HairCluster->GetLink() != FollowerBone ||
		countTestBones(Scene->GetRootNode(), "l_pectoral") != 2)
	{
		LOGTEST_FAILED("A follower component with a unique child was modified.");
		bResult = false;
	}
	Scene->Destroy();
	return bResult;
}

bool UnitTest_DzUnityAction::preserveUnsupportedFollowerCluster(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	FbxScene* Scene = OpenFBXInterface::GetInterface()->CreateScene("UnsupportedFollowerClusterTest");
	FbxNode* Spine = createTestBone(Scene, "spine3", Scene->GetRootNode());
	FbxNode* MainLeft = createTestBone(Scene, "l_pectoral", Spine);
	FbxNode* FollowerLeft = createTestBone(Scene, "l_pectoral", Spine);
	FbxNode* MainRight = createTestBone(Scene, "r_pectoral", Spine);
	FbxNode* FollowerRight = createTestBone(Scene, "r_pectoral", Spine);
	FbxNode* MainMesh = createTestMesh(Scene, "Genesis9.Shape");
	FbxNode* HairMesh = createTestMesh(Scene, "Hair.Shape");
	FbxAMatrix Identity = translatedMatrix(0.0, 0.0, 0.0);
	addTestCluster(Scene, MainMesh, MainLeft, Identity, translatedMatrix(1.0, 0.0, 0.0));
	addTestCluster(Scene, MainMesh, MainRight, Identity, translatedMatrix(2.0, 0.0, 0.0));
	FbxCluster* AdditiveCluster = addTestCluster(
		Scene, HairMesh, FollowerLeft, Identity, translatedMatrix(3.0, 0.0, 0.0), FbxCluster::eAdditive);
	FbxCluster* AssociateCluster = addTestCluster(
		Scene, HairMesh, FollowerRight, Identity, translatedMatrix(4.0, 0.0, 0.0));
	FbxNode* AssociateModel = FbxNode::Create(Scene, "AssociateModel");
	Scene->GetRootNode()->AddChild(AssociateModel);
	AssociateCluster->SetAssociateModel(AssociateModel);
	AssociateCluster->SetTransformAssociateModelMatrix(Identity);

	DzUnityForkAction::FollowerRigConsolidationResult Result =
		qobject_cast<DzUnityForkAction*>(m_testObject)->consolidateFollowerRigs(Scene, QStringList() << "Genesis9.Shape");
	if (!Result.Success || Result.Changed || Result.Warnings.count() < 2 ||
		AdditiveCluster->GetLink() != FollowerLeft || AssociateCluster->GetLink() != FollowerRight)
	{
		LOGTEST_FAILED("Unsupported follower cluster modes were modified.");
		bResult = false;
	}
	Scene->Destroy();
	return bResult;
}

bool UnitTest_DzUnityAction::preserveAmbiguousCanonicalRig(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	FbxScene* Scene = OpenFBXInterface::GetInterface()->CreateScene("AmbiguousCanonicalRigTest");
	FbxNode* Spine = createTestBone(Scene, "spine4", Scene->GetRootNode());
	FbxNode* FirstBone = createTestBone(Scene, "l_shoulder", Spine);
	FbxNode* SecondBone = createTestBone(Scene, "l_shoulder", Spine);
	FbxNode* MainMesh = createTestMesh(Scene, "Genesis9.Shape");
	FbxAMatrix Identity = translatedMatrix(0.0, 0.0, 0.0);
	FbxCluster* FirstCluster = addTestCluster(Scene, MainMesh, FirstBone, Identity, translatedMatrix(1.0, 0.0, 0.0));
	FbxCluster* SecondCluster = addTestCluster(Scene, MainMesh, SecondBone, Identity, translatedMatrix(1.0, 0.0, 0.0));
	FbxNode* FirstProp = FbxNode::Create(Scene, "duplicate_prop");
	FbxNode* SecondProp = FbxNode::Create(Scene, "duplicate_prop");
	Scene->GetRootNode()->AddChild(FirstProp);
	Scene->GetRootNode()->AddChild(SecondProp);

	DzUnityForkAction::FollowerRigConsolidationResult Result =
		qobject_cast<DzUnityForkAction*>(m_testObject)->consolidateFollowerRigs(Scene, QStringList() << "Genesis9.Shape");
	if (!Result.Success || Result.Changed || Result.Warnings.isEmpty() ||
		FirstCluster->GetLink() != FirstBone || SecondCluster->GetLink() != SecondBone ||
		countTestBones(Scene->GetRootNode(), "l_shoulder") != 2)
	{
		LOGTEST_FAILED("An ambiguous canonical bone group was modified.");
		bResult = false;
	}
	Scene->Destroy();
	return bResult;
}

bool UnitTest_DzUnityAction::preserveInconsistentCanonicalBindRig(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	FbxScene* Scene = OpenFBXInterface::GetInterface()->CreateScene("InconsistentCanonicalBindTest");
	FbxNode* Spine = createTestBone(Scene, "spine3", Scene->GetRootNode());
	FbxNode* MainBone = createTestBone(Scene, "l_pectoral", Spine);
	FbxNode* FollowerBone = createTestBone(Scene, "l_pectoral", Spine);
	FbxNode* MainMesh = createTestMesh(Scene, "Genesis9.Shape");
	FbxNode* HairMesh = createTestMesh(Scene, "Hair.Shape");
	FbxAMatrix Identity = translatedMatrix(0.0, 0.0, 0.0);
	addTestCluster(Scene, MainMesh, MainBone, Identity, translatedMatrix(1.0, 0.0, 0.0));
	addTestCluster(Scene, MainMesh, MainBone, Identity, translatedMatrix(2.0, 0.0, 0.0));
	FbxCluster* HairCluster = addTestCluster(Scene, HairMesh, FollowerBone, Identity, translatedMatrix(3.0, 0.0, 0.0));

	DzUnityForkAction::FollowerRigConsolidationResult Result =
		qobject_cast<DzUnityForkAction*>(m_testObject)->consolidateFollowerRigs(Scene, QStringList() << "Genesis9.Shape");
	if (!Result.Success || Result.Changed || Result.Warnings.isEmpty() || HairCluster->GetLink() != FollowerBone)
	{
		LOGTEST_FAILED("A duplicate group with inconsistent canonical bind matrices was modified.");
		bResult = false;
	}
	Scene->Destroy();
	return bResult;
}


#include "moc_UnitTest_DzUnityAction.cpp"

#endif

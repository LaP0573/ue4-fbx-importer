// Fill out your copyright notice in the Description page of Project Settings.

#include <time.h>

#include "ProceduralMeshTests.h"
#include "ProceduralEntity.h"

#include "assimp/DefaultLogger.hpp"
#include "assimp/Logger.hpp"


// Sets default values
AProceduralEntity::AProceduralEntity()
	: _requiresFullRecreation(true), _meshCurrentlyProcessed(0)
{
	this->SetActorEnableCollision(true);
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//_rootComp = CreateDefaultSubobject<USceneComponent>("RootComp");
	//RootComponent = _rootComp;

	_mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProcMesh"));
	if (_mesh) {
		_mesh->CastShadow = true;
		_mesh->SetCollisionObjectType(ECC_WorldDynamic);
		_mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		_mesh->SetCollisionResponseToAllChannels(ECR_Block);
		_mesh->UpdateCollisionProfile();
		RootComponent = _mesh;
	}

	/*if (!IsTemplate(RF_Transient)) {
		std::string filename(TCHAR_TO_UTF8(*_filePath));
		loadModel(filename);
	}*/
}

// Called when the game starts or when spawned
void AProceduralEntity::BeginPlay()
{
	Super::BeginPlay();
}

void AProceduralEntity::PostActorCreated() {
	Super::PostActorCreated();

	if (!IsTemplate(RF_Transient)) {

		std::string filename(TCHAR_TO_UTF8(*_filePath));
		loadModel(filename);
	}
}

// Called every frame
void AProceduralEntity::Tick(float DeltaTime)
{
	Super::Tick( DeltaTime );
	
	struct _stat buf;
	int result;
	std::string sfilename(TCHAR_TO_UTF8(*_filePath));
	// quick C++11 trick to get a char * from a std::string
	char* filename = &sfilename[0];

	result = _stat(filename, &buf);
	if (result != 0) {
		UE_LOG(LogTemp, Warning, TEXT("Problem getting info"));
		switch (errno) {
		case ENOENT:
			UE_LOG(LogTemp, Warning, TEXT("File not found."));
			break;
		case EINVAL:
			UE_LOG(LogTemp, Warning, TEXT("Invalid parameter to _stat."));
			break;
		default:
			UE_LOG(LogTemp, Warning, TEXT("Unexpected error."));
		}
	}
	else {
		if (_lastModifiedTime == 0 || (int)buf.st_mtime > _lastModifiedTime) {
			_lastModifiedTime = (int)buf.st_mtime;
			UE_LOG(LogTemp, Warning, TEXT("Reloading model."));
			loadModel(sfilename);
		}
	}
}

void AProceduralEntity::loadModelFromBlueprint() {
	std::string filename(TCHAR_TO_UTF8(*_filePath));
	loadModel(filename);
}

/* ASSIMP IMPORT */
void AProceduralEntity::processMesh(aiMesh* mesh, const aiScene* scene) {
	UE_LOG(LogTemp, Warning, TEXT("Processing mesh"));

	// the very first time this method runs, we'll need to create the empty arrays
	// we can't really do that in the class constructor because we don't know how many meshes we'll read, and this data can change between imports
	if (_vertices.Num() <= _meshCurrentlyProcessed) {
		_vertices.AddZeroed();
		_normals.AddZeroed();
		_uvs.AddZeroed();
		_tangents.AddZeroed();
		_vertexColors.AddZeroed();
		_indices.AddZeroed();
	}

	// we check whether the current data to read has a different amount of vertices compared to the last time we generated the mesh
	// if so, it means we'll need to recreate the mesh and resupply new indices.
	if (mesh->mNumVertices != _vertices[_meshCurrentlyProcessed].Num())
		_requiresFullRecreation = true;

	// we reinitialize the arrays for the new data we're reading
	_vertices[_meshCurrentlyProcessed].Empty();
	_normals[_meshCurrentlyProcessed].Empty();
	_uvs[_meshCurrentlyProcessed].Empty();
	// this if actually seems useless, seeing what it does without it
	//if (_requiresFullRecreation) {
		_tangents[_meshCurrentlyProcessed].Empty();
		_vertexColors[_meshCurrentlyProcessed].Empty();
		_indices[_meshCurrentlyProcessed].Empty();
	//}

	for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
		FVector vertex, normal;
		// process vertex positions, normals and UVs
		vertex.X = mesh->mVertices[i].x;
		vertex.Y = mesh->mVertices[i].y;
		vertex.Z = mesh->mVertices[i].z;

		normal.X = mesh->mNormals[i].x;
		normal.Y = mesh->mNormals[i].y;
		normal.Z = mesh->mNormals[i].z;

		// if the mesh contains tex coords
		if (mesh->mTextureCoords[0]) {
			FVector2D uvs;
			uvs.X = mesh->mTextureCoords[0][i].x;
			uvs.Y = mesh->mTextureCoords[0][i].y;
			_uvs[_meshCurrentlyProcessed].Add(uvs);
		}
		else {
			_uvs[_meshCurrentlyProcessed].Add(FVector2D(0.f, 0.f));
		}
		_vertices[_meshCurrentlyProcessed].Add(vertex);
		_normals[_meshCurrentlyProcessed].Add(normal);
	}

	if (_requiresFullRecreation) {
		// process indices
		for (uint32 i = 0; i < mesh->mNumFaces; i++) {
			aiFace face = mesh->mFaces[i];
			_indices[_meshCurrentlyProcessed].Add(face.mIndices[2]);
			_indices[_meshCurrentlyProcessed].Add(face.mIndices[1]);
			_indices[_meshCurrentlyProcessed].Add(face.mIndices[0]);
		}
	}

	// we can finally either update or create the mesh
	if (_requiresFullRecreation) {
		UE_LOG(LogTemp, Warning, TEXT("Full recreation"));
		_mesh->CreateMeshSection(_meshCurrentlyProcessed, _vertices[_meshCurrentlyProcessed], _indices[_meshCurrentlyProcessed], _normals[_meshCurrentlyProcessed], _uvs[_meshCurrentlyProcessed], _vertexColors[_meshCurrentlyProcessed], _tangents[_meshCurrentlyProcessed], true);
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("Update"));
		_mesh->UpdateMeshSection(_meshCurrentlyProcessed, _vertices[_meshCurrentlyProcessed], _normals[_meshCurrentlyProcessed], _uvs[_meshCurrentlyProcessed], _vertexColors[_meshCurrentlyProcessed], _tangents[_meshCurrentlyProcessed]);
	}

	UE_LOG(LogTemp, Warning, TEXT("Done creating the mesh"));

	_requiresFullRecreation = false;
}

void AProceduralEntity::processNode(aiNode* node, const aiScene* scene) {
	UE_LOG(LogTemp, Warning, TEXT("Processing a node"));
	// process all the node's meshes
	for (uint32 i = 0; i < node->mNumMeshes; i++) {
		UE_LOG(LogTemp, Warning, TEXT("New mesh in the node"));
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		processMesh(mesh, scene);
		++_meshCurrentlyProcessed;
	}

	// do the same for all of its children
	for (uint32 i = 0; i < node->mNumChildren; i++) {
		UE_LOG(LogTemp, Warning, TEXT("New child in the node"));
		processNode(node->mChildren[i], scene);
	}
}

void AProceduralEntity::loadModel(std::string path) {
	Assimp::Importer importer;

	UE_LOG(LogTemp, Warning, TEXT("START LOGGING"));
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);

	if (!scene)
		return;

	_meshCurrentlyProcessed = 0;
	processNode(scene->mRootNode, scene);
}
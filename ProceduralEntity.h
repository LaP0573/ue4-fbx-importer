// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <string>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "ProceduralEntity.generated.h"

UCLASS()
class PROCEDURALMESHTESTS_API AProceduralEntity : public AActor
{
	GENERATED_BODY()
	
public:	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	UProceduralMeshComponent* _mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	FString _filePath;

	// Sets default values for this actor's properties
	AProceduralEntity();

	void PostActorCreated();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	UFUNCTION(BlueprintCallable, Category = ProceduralEntity)
	void loadModelFromBlueprint();

private:
	int32 _selectedVertex;
	int32 _meshCurrentlyProcessed;
	bool _addModifier;
	int _lastModifiedTime;
	bool _requiresFullRecreation;

	TArray<TArray<FVector>> _vertices;
	TArray<TArray<int32>> _indices;
	TArray<TArray<FVector>> _normals;
	TArray<TArray<FVector2D>> _uvs;
	TArray<TArray<FProcMeshTangent>> _tangents;
	TArray<TArray<FColor>> _vertexColors;

	//USceneComponent* _rootComp;

	/* ################################################### */
	/* ################# Input methods ################### */
	/* ################################################### */
	/*
	template<int vertexNb>
	void SelectVertex();
	template<bool addModifier>
	void ChangeAddModifier();
	void ChangeVertex(FVector dv);*/

	void processMesh(aiMesh* mesh, const aiScene* scene);
	void processNode(aiNode* node, const aiScene* scene);
	void loadModel(std::string path);
};
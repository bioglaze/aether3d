#include "SpotLightInspector.hpp"
#include "GameObject.hpp"
#include "SpotLightComponent.hpp"
#include "System.hpp"
#include <QLabel>
#include <QBoxLayout>
#include <QCheckBox>

void SpotLightInspector::Init( QWidget* mainWindow )
{
    auto componentName = new QLabel( "Spot Light" );
    auto shadowCheck = new QCheckBox( "Casts Shadow" );

    auto inspectorLayout = new QBoxLayout( QBoxLayout::TopToBottom );
    inspectorLayout->setContentsMargins( 1, 1, 1, 1 );
    inspectorLayout->addWidget( componentName );
    inspectorLayout->addWidget( shadowCheck );

    root = new QWidget();
    root->setLayout( inspectorLayout );

    connect( shadowCheck, SIGNAL(stateChanged(int)), this, SLOT(ShadowStateChanged(int)) );
    connect( mainWindow, SIGNAL(GameObjectSelected(std::list< ae3d::GameObject* >)),
             this, SLOT(GameObjectSelected(std::list< ae3d::GameObject* >)) );
}

void SpotLightInspector::ShadowStateChanged( int enabled )
{
    ae3d::System::Assert( gameObject, "Needs game object" );
    ae3d::System::Assert( gameObject->GetComponent< ae3d::SpotLightComponent >() != nullptr, "Needs dir light" );

    gameObject->GetComponent< ae3d::SpotLightComponent >()->SetCastShadow( enabled == 2 ? true : false, 512 );
}

void SpotLightInspector::GameObjectSelected( std::list< ae3d::GameObject* > gameObjects )
{
    if (gameObjects.empty())
    {
        gameObject = nullptr;
    }
    else
    {
        gameObject = gameObjects.front();
    }
}

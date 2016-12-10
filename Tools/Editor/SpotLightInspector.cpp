#include "SpotLightInspector.hpp"
#include <iomanip>
#include <QLabel>
#include <QIntValidator>
#include <QBoxLayout>
#include <QCheckBox>
#include <QColorDialog>
#include <QPushButton>
#include <QLineEdit>
#include "GameObject.hpp"
#include "SpotLightComponent.hpp"
#include "System.hpp"

QString FloatToQString( float f );

void SpotLightInspector::Init( QWidget* mainWindow )
{
    auto componentName = new QLabel( "Spot Light" );
    shadowCheck = new QCheckBox( "Casts Shadow" );

    removeButton = new QPushButton("remove");
    QBoxLayout* headerLayout = new QBoxLayout( QBoxLayout::LeftToRight );
    headerLayout->addWidget( componentName );
    headerLayout->addWidget( removeButton );
    QWidget* header = new QWidget();
    header->setLayout( headerLayout );

    QLabel* coneAngleTitle = new QLabel( "Cone angle" );

    coneAngleInput = new QLineEdit();
    coneAngleInput->setMaxLength( 3 );
    coneAngleInput->setMaximumWidth( 40 );
    coneAngleInput->setValidator( new QIntValidator() );

    QWidget* coneWidget = new QWidget();
    QBoxLayout* coneLayout = new QBoxLayout( QBoxLayout::LeftToRight );
    coneLayout->addWidget( coneAngleTitle );
    coneLayout->addWidget( coneAngleInput );
    coneWidget->setLayout( coneLayout );

    colorButton = new QPushButton( "color" );

    QWidget* colorWidget = new QWidget();
    QBoxLayout* colorLayout = new QBoxLayout( QBoxLayout::LeftToRight );
    colorLayout->addWidget( colorButton );
    colorWidget->setLayout( colorLayout );

    auto inspectorLayout = new QBoxLayout( QBoxLayout::TopToBottom );
    inspectorLayout->setContentsMargins( 1, 1, 1, 1 );
    inspectorLayout->addWidget( header );
    inspectorLayout->addWidget( coneWidget );
    inspectorLayout->addWidget( colorWidget );
    inspectorLayout->addWidget( shadowCheck );

    root = new QWidget();
    root->setLayout( inspectorLayout );

    colorDialog = new QColorDialog( root );

    connect( shadowCheck, SIGNAL(stateChanged(int)), this, SLOT(ShadowStateChanged(int)) );
    connect( mainWindow, SIGNAL(GameObjectSelected(std::list< ae3d::GameObject* >)),
             this, SLOT(GameObjectSelected(std::list< ae3d::GameObject* >)) );
    connect( removeButton, SIGNAL(clicked(bool)), mainWindow, SLOT(CommandRemoveSpotLightComponent()));
    connect( colorButton, &QPushButton::clicked, [=]{ colorDialog->show(); } );
    connect( colorDialog, &QColorDialog::colorSelected,
        [=]( const QColor& color )
        {
            colorDialog->hide();
            gameObject->GetComponent< ae3d::SpotLightComponent >()->SetColor( ae3d::Vec3( color.redF(), color.greenF(), color.blueF() ) );
        });

    connect( colorDialog, &QColorDialog::currentColorChanged,
        [=]( const QColor& color )
        {
            gameObject->GetComponent< ae3d::SpotLightComponent >()->SetColor( ae3d::Vec3( color.redF(), color.greenF(), color.blueF() ) );
        });

    connect( coneAngleInput, &QLineEdit::textChanged,
        [&](const QString& angle)
        {
            try
            {
                float coneAngle = std::stof( angle.toStdString().c_str() );
                gameObject->GetComponent< ae3d::SpotLightComponent >()->SetConeAngle( coneAngle );
            }
            catch (std::invalid_argument&)
            {
            }
        });
}

void SpotLightInspector::ShadowStateChanged( int enabled )
{
    ae3d::System::Assert( gameObject, "Needs game object" );
    ae3d::System::Assert( gameObject->GetComponent< ae3d::SpotLightComponent >() != nullptr, "Needs spot light" );

    gameObject->GetComponent< ae3d::SpotLightComponent >()->SetCastShadow( enabled != 0, 1024 );
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

        auto spotLight = gameObject->GetComponent< ae3d::SpotLightComponent >();

        if (spotLight != nullptr)
        {
            shadowCheck->setChecked( spotLight->CastsShadow() );
            coneAngleInput->setText( FloatToQString( spotLight->GetConeAngle() ) );
        }
    }
}

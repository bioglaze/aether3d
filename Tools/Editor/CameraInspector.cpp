#include "CameraInspector.hpp"
#include <iostream>
#include <QBoxLayout>
#include <QColorDialog>
#include <QTableWidget>
#include <QLabel>
#include <QComboBox>
#include <QString>
#include <QHeaderView>
#include <QPushButton>
#include "GameObject.hpp"
#include "CameraComponent.hpp"
#include "Quaternion.hpp"
#include "System.hpp"

using namespace ae3d;

void CameraInspector::Init( QWidget* mainWindow )
{
    ortho = new QTableWidget( 1, 4 );
    //ortho->horizontalHeader()->setDefaultSectionSize( 15 );
    ortho->setItem( 0, 0, new QTableWidgetItem() );
    ortho->setItem( 0, 1, new QTableWidgetItem() );
    ortho->setItem( 0, 2, new QTableWidgetItem() );
    ortho->setItem( 0, 3, new QTableWidgetItem() );
    ortho->setHorizontalHeaderLabels( QString("Width;Height;Near;Far").split(";") );
    ortho->setVerticalHeaderLabels( QString("Orthographic").split(";") );

    persp = new QTableWidget( 1, 4 );
    persp->setItem( 0, 0, new QTableWidgetItem() );
    persp->setItem( 0, 1, new QTableWidgetItem() );
    persp->setItem( 0, 2, new QTableWidgetItem() );
    persp->setItem( 0, 3, new QTableWidgetItem() );
    persp->setHorizontalHeaderLabels( QString("Fov;Aspect;Near;Far").split(";") );
    persp->setVerticalHeaderLabels( QString("Perspective").split(";") );

    QLabel* componentName = new QLabel("Camera");
    QLabel* clearTitle = new QLabel("Clear");
    QLabel* projectionTitle = new QLabel("Projection");

    clearFlagsBox = new QComboBox();
    clearFlagsBox->addItem("Color and depth");
    clearFlagsBox->addItem("Don't clear");

    projectionBox = new QComboBox();
    projectionBox->addItem("Orthographic");
    projectionBox->addItem("Perspective");

    //clearColorDialog = new QColorDialog();
    clearColorButton = new QPushButton("clear color");

    QBoxLayout* clearLayout = new QBoxLayout( QBoxLayout::LeftToRight );
    //clearLayout->setMargin(1);
    clearLayout->setContentsMargins( 1, 1, 1, 1 );
    clearLayout->addWidget( clearTitle );
    clearLayout->addWidget( clearFlagsBox );
    clearLayout->addWidget( clearColorButton );
    //clearLayout->addWidget( clearColorDialog );
    QWidget* clearWidget = new QWidget();
    clearWidget->setLayout( clearLayout );

    QBoxLayout* projectionLayout = new QBoxLayout( QBoxLayout::LeftToRight );
    projectionLayout->setContentsMargins( 1, 1, 1, 1 );
    projectionLayout->addWidget( projectionTitle );
    projectionLayout->addWidget( projectionBox );
    QWidget* projectionWidget = new QWidget();
    projectionWidget->setLayout( projectionLayout );

    root = new QWidget();
    QBoxLayout* inspectorLayout = new QBoxLayout( QBoxLayout::TopToBottom );
    inspectorLayout->setContentsMargins( 1, 1, 1, 1 );
    inspectorLayout->addWidget( componentName );
    inspectorLayout->addWidget( clearWidget );
    inspectorLayout->addWidget( projectionWidget );
    inspectorLayout->addWidget( ortho );
    inspectorLayout->addWidget( persp );

    root = new QWidget();
    root->setLayout( inspectorLayout );

    connect( mainWindow, SIGNAL(GameObjectSelected(std::list< ae3d::GameObject* >)),
             this, SLOT(GameObjectSelected(std::list< ae3d::GameObject* >)) );
    connect( ortho, &QTableWidget::itemChanged, [&](QTableWidgetItem* /*item*/) { ApplyFieldsIntoSelectedCamera(); });
    connect( persp, &QTableWidget::itemChanged, [&](QTableWidgetItem* /*item*/) { ApplyFieldsIntoSelectedCamera(); });
    connect( clearFlagsBox, SIGNAL(currentIndexChanged(int)), this, SLOT(ClearFlagsChanged(int) ));
    connect( projectionBox, SIGNAL(currentIndexChanged(int)), this, SLOT(ProjectionChanged() ));
    connect( clearColorButton, SIGNAL(clicked()), this, SLOT(OpenColorSelection() ));
}

void CameraInspector::OpenColorSelection()
{
    new QColorDialog();
}

void CameraInspector::ProjectionChanged()
{
    ApplyFieldsIntoSelectedCamera();
}

void CameraInspector::ClearFlagsChanged( int )
{
    ApplyFieldsIntoSelectedCamera();
}

void CameraInspector::ApplySelectedCameraIntoFields( const ae3d::CameraComponent& camera )
{
    disconnect( ortho, &QTableWidget::itemChanged, ortho, nullptr );
    disconnect( persp, &QTableWidget::itemChanged, persp, nullptr );

    projectionBox->setCurrentIndex( camera.GetProjectionType() == ae3d::CameraComponent::ProjectionType::Orthographic ? 0 : 1 );
    clearFlagsBox->setCurrentIndex( camera.GetClearFlag() == ae3d::CameraComponent::ClearFlag::DepthAndColor ? 0 : 1 );

    persp->item( 0, 0 )->setText( QString::fromStdString( std::to_string( camera.GetFovDegrees() ) ) );
    persp->item( 0, 1 )->setText( QString::fromStdString( std::to_string( camera.GetAspect() ) ) );
    persp->item( 0, 2 )->setText( QString::fromStdString( std::to_string( camera.GetNear() ) ) );
    persp->item( 0, 3 )->setText( QString::fromStdString( std::to_string( camera.GetFar() ) ) );

    ortho->item( 0, 0 )->setText( QString::fromStdString( std::to_string( camera.GetRight() ) ) );
    ortho->item( 0, 1 )->setText( QString::fromStdString( std::to_string( camera.GetTop() ) ) );
    ortho->item( 0, 2 )->setText( QString::fromStdString( std::to_string( camera.GetNear() ) ) );
    ortho->item( 0, 3 )->setText( QString::fromStdString( std::to_string( camera.GetFar() ) ) );

    connect( ortho, &QTableWidget::itemChanged, [&](QTableWidgetItem* /*item*/) { ApplyFieldsIntoSelectedCamera(); });
    connect( persp, &QTableWidget::itemChanged, [&](QTableWidgetItem* /*item*/) { ApplyFieldsIntoSelectedCamera(); });
}

void CameraInspector::ApplyFieldsIntoSelectedCamera()
{
    float width, height, fov, aspect, orthoNear, orthoFar, perspNear, perspFar;

    try
    {
        fov = std::stof( persp->item( 0, 0 )->text().toUtf8().constData() );
    }
    catch (std::invalid_argument&)
    {
        fov = 0;
    }

    try
    {
        aspect = std::stof( persp->item( 0, 1 )->text().toUtf8().constData() );
    }
    catch (std::invalid_argument&)
    {
        aspect = 1;
    }

    try
    {
        perspNear = std::stof( persp->item( 0, 2 )->text().toUtf8().constData() );
    }
    catch (std::invalid_argument&)
    {
        perspNear = 1;
    }

    try
    {
        perspFar = std::stof( persp->item( 0, 3 )->text().toUtf8().constData() );
    }
    catch (std::invalid_argument&)
    {
        perspFar = 1;
    }

    try
    {
        width = std::stof( ortho->item( 0, 0 )->text().toUtf8().constData() );
    }
    catch (std::invalid_argument&)
    {
        width = 1;
    }

    try
    {
        height = std::stof( ortho->item( 0, 1 )->text().toUtf8().constData() );
    }
    catch (std::invalid_argument&)
    {
        height = 1;
    }


    try
    {
        orthoNear = std::stof( ortho->item( 0, 2 )->text().toUtf8().constData() );
    }
    catch (std::invalid_argument&)
    {
        orthoNear = 1;
    }

    try
    {
        orthoFar = std::stof( ortho->item( 0, 3 )->text().toUtf8().constData() );
    }
    catch (std::invalid_argument&)
    {
        orthoFar = 1;
    }

    const ae3d::Vec4 orthoParams { width, height, orthoNear, orthoFar };
    const ae3d::Vec4 perspParams { fov, aspect, perspNear, perspFar };

    ae3d::CameraComponent::ProjectionType projectionType = ae3d::CameraComponent::ProjectionType::Perspective;

    emit CameraModified( CameraComponent::ClearFlag::DepthAndColor, projectionType, orthoParams, perspParams );
}

void CameraInspector::GameObjectSelected( std::list< ae3d::GameObject* > gameObjects )
{
    if (gameObjects.empty() || !gameObjects.front()->GetComponent< ae3d::CameraComponent >())
    {
        return;
    }

    disconnect( ortho, &QTableWidget::itemChanged, ortho, nullptr );
    disconnect( persp, &QTableWidget::itemChanged, persp, nullptr );
    ApplySelectedCameraIntoFields( *gameObjects.front()->GetComponent< ae3d::CameraComponent >() );
    connect( ortho, &QTableWidget::itemChanged, [&](QTableWidgetItem*) { ApplyFieldsIntoSelectedCamera(); });
    connect( persp, &QTableWidget::itemChanged, [&](QTableWidgetItem*) { ApplyFieldsIntoSelectedCamera(); });
}

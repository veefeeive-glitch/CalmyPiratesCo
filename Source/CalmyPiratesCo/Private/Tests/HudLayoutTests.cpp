#include "Core/CalmyHudLayout.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyHudLayoutNoOverlap,"CalmyPiratesCo.UI.HudLayoutNoOverlap",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCalmyHudLayoutNoOverlap::RunTest(const FString&)
{
    // Worst minimal HUD: leak alert, toast and tip on top, long prompt, hurt vitals, two-line subtitle, FPS.
    FCalmyHudLayoutInput Busy;
    Busy.Top={FVector2D(620,62),FVector2D(640,40),FVector2D(560,36)};
    Busy.Prompt=FVector2D(720,36);
    Busy.Subtitle=FVector2D(900,72);
    Busy.Vitals=FVector2D(300,36);
    Busy.Fps=FVector2D(170,20);
    // Reference viewports: 16:9, 4:3, 21:9, 32:9, HUD scale 1.5 (1080 / 1.5 = 720 units) and small windows.
    const FVector2D Viewports[]={{1920,1080},{1440,1080},{2520,1080},{3840,1080},{1280,720},{1024,768},{800,600}};
    for(const FVector2D& Viewport:Viewports)
    {
        FCalmyHudLayoutInput In=Busy;In.Viewport=Viewport;In.PromptTop=Viewport.Y*.5f+22;
        const FCalmyHudLayout Layout=FCalmyHudLayoutSolver::Solve(In);
        const FString Name=FString::Printf(TEXT("%.0fx%.0f"),Viewport.X,Viewport.Y);
        TestTrue(TEXT("no overlap and inside ")+Name,FCalmyHudLayoutSolver::IsValid(Layout,Viewport));
        TestTrue(TEXT("alert survives ")+Name,Layout.Top.Num()>=1);
        TestFalse(TEXT("prompt survives ")+Name,Layout.Prompt.IsEmpty());
    }
    FCalmyHudLayoutInput Full=Busy;Full.Viewport=FVector2D(1920,1080);Full.PromptTop=562;
    const FCalmyHudLayout Wide=FCalmyHudLayoutSolver::Solve(Full);
    TestEqual(TEXT("all top messages at 1080p"),Wide.Top.Num(),3);
    TestFalse(TEXT("subtitle visible at 1080p"),Wide.Subtitle.IsEmpty());
    TestEqual(TEXT("vitals bottom-left"),Wide.Vitals.X,FCalmyHudLayoutSolver::Margin);
    TestTrue(TEXT("top messages stack downward"),Wide.Top[0].Y<Wide.Top[1].Y&&Wide.Top[1].Y<Wide.Top[2].Y);

    FCalmyHudLayoutInput Narrow=Busy;Narrow.Viewport=FVector2D(1024,768);Narrow.PromptTop=406;
    const FCalmyHudLayout Small=FCalmyHudLayoutSolver::Solve(Narrow);
    TestTrue(TEXT("subtitle raised above vitals on a narrow screen"),Small.Subtitle.IsEmpty()||Small.Subtitle.Bottom()<=Small.Vitals.Y);

    FCalmyHudLayoutInput Stacked=Busy;Stacked.Viewport=FVector2D(1280,720);Stacked.PromptTop=382;
    for(int32 I=0;I<4;++I)Stacked.Top.Add(FVector2D(600,60));
    const FCalmyHudLayout Clipped=FCalmyHudLayoutSolver::Solve(Stacked);
    TestTrue(TEXT("low-priority top messages dropped"),Clipped.Top.Num()<Stacked.Top.Num());
    TestTrue(TEXT("clipped layout valid"),FCalmyHudLayoutSolver::IsValid(Clipped,Stacked.Viewport));

    FCalmyHudLayoutInput Empty;Empty.Viewport=FVector2D(1920,1080);
    const FCalmyHudLayout None=FCalmyHudLayoutSolver::Solve(Empty);
    TestTrue(TEXT("empty input draws nothing"),None.Top.Num()==0&&None.Prompt.IsEmpty()&&None.Subtitle.IsEmpty()&&None.Vitals.IsEmpty()&&None.Fps.IsEmpty());
    return true;
}
#endif

#include "StreamRecorder.h"
#include "StreamPlayer.h"
#include "miniosc.h"

#include "ofxOsc.h"

#import <Cocoa/Cocoa.h>

@interface AppDelegate : NSObject <NSApplicationDelegate>
{
	NSWindow *window;
	NSArrayController *hostArrayController;
	NSTextField *port;
	NSTextField *osc_input_status;
	NSSlider *playhead;
	
	StreamRecorder recorder;
	StreamPlayer player;
	
	NSTextField *osc_output_status;
	
	ofxOscReceiver control_osc_input;
	
	BOOL playing;
	BOOL recording;
	BOOL loaded;
	BOOL opend;
	BOOL osc_through_output;
	int control_osc_port;
}

@property (assign) BOOL playing;
@property (assign) BOOL recording;
@property (assign) BOOL loaded;
@property (assign) BOOL opend;
@property (assign) BOOL osc_through_output;

@property (assign) IBOutlet NSWindow *window;
@property (assign) IBOutlet NSArrayController *hostArrayController;

- (void)load:(NSString*)path;

@property (assign) IBOutlet NSTextField *port;
@property (assign) IBOutlet NSTextField *osc_input_status;
@property (assign) IBOutlet NSTextField *osc_output_status;

- (IBAction)onOpenRecording:(id)sender;

// playback
@property (assign) IBOutlet NSSlider *playhead;

- (IBAction)onLoad:(id)sender;
- (IBAction)onPlayHeadChanged:(id)sender;

- (IBAction)setControlOscPort:(id)sender;
- (IBAction)reloadHostXML:(id)sender;

//

- (IBAction)inputPortChanged:(id)sender;

@end

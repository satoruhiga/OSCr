#import "AppDelegate.h"

@interface HostModel : NSObject
{
	NSString *ip;
	NSNumber *port;
}

@property(retain) NSString *ip;
@property(retain) NSNumber *port;

@end

@implementation HostModel

@synthesize ip;
@synthesize port;

@end

//

@implementation AppDelegate

@synthesize hostArrayController;

@synthesize window;
@synthesize port;
@synthesize osc_input_status;
@synthesize osc_output_status;
@synthesize playhead;
@synthesize osc_through_output;

string save_path;

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
	ofSetLogLevel(OF_LOG_VERBOSE);
	
	player.setLoopState(true);
	recorder.open([port intValue]);
	
	NSString *path = [[NSBundle mainBundle] bundlePath];
	path = [path stringByDeletingLastPathComponent];
	path = [path stringByAppendingPathComponent:@"data"];
	path = [path stringByAppendingString:@"/"];
	ofSetDataPathRoot([path UTF8String]);
	
	[NSTimer scheduledTimerWithTimeInterval:1. / 15 target:self selector:@selector(onGuiLoop:) userInfo:nil repeats:YES];
	
	[self reloadHostXML:nil];
	
	[self addObserver:self forKeyPath:@"playing" options:0 context:NULL];
	[self addObserver:self forKeyPath:@"recording" options:0 context:NULL];
	[self addObserver:self forKeyPath:@"loaded" options:0 context:NULL];
	
	NSUserDefaults *def = [NSUserDefaults standardUserDefaults];
	
	[def addObserver:self forKeyPath:@"control_osc_port" options:0 context:NULL];

	{
		int control_osc_port_ = [def integerForKey:NSStringFromSelector(@selector(control_osc_port))];
		control_osc_input.setup(control_osc_port_);
	}
	
	{
		recorder.setEnableThroughOut([def boolForKey:NSStringFromSelector(@selector(osc_through_output))]);
	}
}

- (void)dealloc
{
    [super dealloc];
}

// KVO

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
	if ([object isEqualTo:self])
	{
		if ([keyPath isEqualToString:NSStringFromSelector(@selector(playing))])
		{
			if (self.playing) self.recording = NO;
		}
		else if ([keyPath isEqualToString:NSStringFromSelector(@selector(recording))])
		{
			if (self.recording) self.playing = NO;
		}
		else if ([keyPath isEqualToString:NSStringFromSelector(@selector(loaded))])
		{
			[self.playhead setEnabled:self.loaded];
		}
	}
	else if ([object isEqualTo:[NSUserDefaults standardUserDefaults]])
	{
		NSUserDefaults *def = [NSUserDefaults standardUserDefaults];
		if ([keyPath isEqualToString:NSStringFromSelector(@selector(control_osc_port))])
		{
			int control_osc_port_ = [def integerForKey:NSStringFromSelector(@selector(control_osc_port))];
			control_osc_input.setup(control_osc_port_);
			ofLogVerbose("OSCr") << "control osc port setup: " << control_osc_port_;
		}
	}
}

- (BOOL)playing
{
	return player.isPlaying();
}

- (void)setPlaying:(BOOL)playing_
{
	if (playing_) player.start();
	else player.stop();
}

- (BOOL)recording
{
	return recorder.isRecording();
}

- (void)setRecording:(BOOL)recording_
{
	if (recording_) recorder.start(save_path, true);
	else recorder.stop();
}

- (BOOL)loaded
{
	return loaded;
}

- (void)setLoaded:(BOOL)loaded_
{
	loaded = loaded_;
}

- (BOOL)opend
{
	return opend;
}

- (void)setOpend:(BOOL)opend_
{
	opend = opend_;
}

- (BOOL)osc_through_output
{
	return [[[NSUserDefaults standardUserDefaults] objectForKey:NSStringFromSelector(@selector(osc_through_output))] boolValue];
}

- (void)setOsc_through_output:(BOOL)osc_through_output_
{
	[[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithBool:osc_through_output_] forKey:NSStringFromSelector(@selector(osc_through_output))];
	recorder.setEnableThroughOut(osc_through_output_);
	osc_through_output = osc_through_output_;
}

- (id)onGuiLoop:(id)sender
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
	string data = recorder.getLatestMessage();
	if (data.size())
	{
		MiniOSC::Reader reader(data);
		
		stringstream ss;
		ss << reader.getAddress() << " " << reader.getTypeTags();
		[self.osc_input_status setStringValue:[NSString stringWithUTF8String:ss.str().c_str()]];
	}
	
	if (self.playing)
	{
		float d = player.getPosition() / player.getDuration();
		[self.playhead setFloatValue:d * 1000];
		
		[self.osc_output_status setStringValue:[NSString stringWithUTF8String:player.getLatestMessage().c_str()]];
	}
	
	{
		NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
		
		while (control_osc_input.hasWaitingMessages())
		{
			ofxOscMessage m;
			control_osc_input.getNextMessage(&m);
			
			string addr = m.getAddress();
			
			if (addr == "/record")
			{
				save_path = "";
				
				if (m.getNumArgs() > 0)
					save_path = m.getArgAsString(0);
				
				self.opend = YES;
				self.recording = YES;
			}
			else if (addr == "/play")
			{
				self.playing = YES;
			}
			else if (addr == "/load")
			{
				if (m.getNumArgs() > 0)
				{
					string path = m.getArgAsString(0);
					[self load:[NSString stringWithUTF8String:path.c_str()]];
				}
			}
			else if (addr == "/stop")
			{
				self.playing = NO;
			}
			else if (addr == "/through")
			{
				int n = m.getArgAsInt32(0);
				self.osc_through_output = n;
			}
		}
	}
	
	[pool drain];
}

//

- (IBAction)inputPortChanged:(id)sender {
	int osc_port = [self.port intValue];
	if (osc_port > 1024 && osc_port < 65535)
		recorder.open(osc_port);
}

//

- (void)load:(NSString*)path_
{
	self.playing = NO;
	
	string path = [path_ UTF8String];
	if (player.open(path))
	{
		self.loaded = YES;
		[self.playhead setFloatValue:0];
	}
	else
	{
		self.loaded = NO;
	}
}

- (IBAction)onPlayHeadChanged:(id)sender {
	float d = (float)[sender intValue] / (float)[sender maxValue];
	float p = player.getDuration() * d;
	player.setPosition(p);
}

- (IBAction)onLoad:(id)sender {
	NSOpenPanel *panel = [NSOpenPanel openPanel];
	
	[panel setAllowsMultipleSelection:NO];
	[panel setCanChooseDirectories:NO];
	[panel setAllowedFileTypes:@[@"sqlite"]];
	
	[panel beginWithCompletionHandler:^(NSInteger result) {
		if (result == NSFileHandlingPanelOKButton)
		{
			[self load:[[panel URL] path]];
		}
	}];
}

- (IBAction)onOpenRecording:(id)sender {
	NSSavePanel *panel = [NSSavePanel savePanel];
	
	[panel setAllowedFileTypes:@[@"sqlite"]];
	
	[panel beginWithCompletionHandler:^(NSInteger result) {
		if (result == NSFileHandlingPanelOKButton)
		{
			save_path = [[[panel URL] path] UTF8String];
			self.opend = YES;
		}
	}];
}

// setting window

- (IBAction)setControlOscPort:(id)sender {
	control_osc_input.setup([sender intValue]);
}

- (IBAction)reloadHostXML:(id)sender
{
	string filename = "host.xml";
	ofFile file(filename);
	
	if (!file.exists())
	{
		ofBuffer buf("<hosts><host ip='localhost' port='10000' /></hosts>");
		ofBufferToFile(filename, buf);
	}
	
	player.clearHost();
	
	ofXml xml;
	xml.load(filename);
	
	stringstream ss;
	
	NSMutableArray *arr = [NSMutableArray array];
	
	for (int i = 0; i < xml.getNumChildren("host"); i++)
	{
		string tag = "host[" + ofToString(i) + "]";
		
		string target_ip = xml.getAttribute(tag + "[@ip]");
		int target_port = ofToInt(xml.getAttribute(tag + "[@port]"));
		
		player.addHost(target_ip, target_port);
		
		HostModel *o = [[[HostModel alloc] init] autorelease];
		o.ip = [NSString stringWithUTF8String:target_ip.c_str()];
		o.port = [NSNumber numberWithInt:target_port];
		
		[arr addObject:o];
	}
	
	[self.hostArrayController setContent:arr];
	
	recorder.setThroughAddresses(player.getAddresses());
}

@end

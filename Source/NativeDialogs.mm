#include "NativeDialogs.h"

#if JUCE_MAC
#import <Cocoa/Cocoa.h>

// Helper class to handle menu item selection
@interface PresetMenuHandler : NSObject
@property (nonatomic, copy) void (^callback)(int);
@property (nonatomic, assign) int selectedValue;
- (void)menuItemSelected:(id)sender;
@end

@implementation PresetMenuHandler
- (void)menuItemSelected:(id)sender {
    NSMenuItem *item = (NSMenuItem *)sender;
    self.selectedValue = (int)[item tag];
    if (self.callback) {
        self.callback(self.selectedValue);
    }
}
@end

juce::File NativeDialogs::getPresetsFolder()
{
    // Get user's Documents folder and create XYControl Presets folder
    juce::File documentsDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
    juce::File presetsFolder = documentsDir.getChildFile("XYControl Presets");

    if (!presetsFolder.exists())
        presetsFolder.createDirectory();

    return presetsFolder;
}

void NativeDialogs::showSaveDialog(const juce::File& presetsFolder, std::function<void(juce::File)> callback)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        NSSavePanel *panel = [NSSavePanel savePanel];
        [panel setTitle:@"Save Preset"];
        [panel setPrompt:@"Save"];
        [panel setNameFieldStringValue:@"my_preset.json"];
        [panel setAllowedFileTypes:@[@"json"]];
        [panel setDirectoryURL:[NSURL fileURLWithPath:[NSString stringWithUTF8String:presetsFolder.getFullPathName().toRawUTF8()]]];
        [panel setCanCreateDirectories:YES];
        [panel setShowsTagField:NO];

        NSModalResponse result = [panel runModal];

        if (result == NSModalResponseOK) {
            NSURL *fileURL = [panel URL];
            NSString *path = [fileURL path];
            juce::File file(juce::String::fromUTF8([path UTF8String]));
            callback(file);
        } else {
            callback(juce::File());
        }
    });
}

void NativeDialogs::showPresetBrowser(const juce::File& presetsFolder,
                                      std::function<void(juce::File)> onLoad)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        NSOpenPanel *panel = [NSOpenPanel openPanel];
        [panel setTitle:@"Load Preset"];
        [panel setPrompt:@"Load"];
        [panel setCanChooseFiles:YES];
        [panel setCanChooseDirectories:NO];
        [panel setAllowsMultipleSelection:NO];
        [panel setCanCreateDirectories:YES];
        [panel setShowsTagField:NO];
        [panel setAllowedFileTypes:@[@"json"]];
        [panel setDirectoryURL:[NSURL fileURLWithPath:[NSString stringWithUTF8String:presetsFolder.getFullPathName().toRawUTF8()]]];

        // Add accessory view with "New Folder" button
        NSView *accessoryView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 300, 30)];
        NSButton *newFolderButton = [[NSButton alloc] initWithFrame:NSMakeRect(0, 0, 120, 24)];
        [newFolderButton setTitle:@"New Folder..."];
        [newFolderButton setBezelStyle:NSBezelStyleRounded];
        [newFolderButton setTarget:nil];
        [newFolderButton setAction:nil]; // Action handled separately
        [accessoryView addSubview:newFolderButton];
        [panel setAccessoryView:accessoryView];

        NSModalResponse result = [panel runModal];

        if (result == NSModalResponseOK) {
            NSURL *fileURL = [[panel URLs] firstObject];
            NSString *path = [fileURL path];
            juce::File file(juce::String::fromUTF8([path UTF8String]));
            onLoad(file);
        } else {
            onLoad(juce::File());
        }
    });
}

void NativeDialogs::createNewFolder(const juce::File& parentFolder, std::function<void(bool)> callback)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"New Folder"];
        [alert setInformativeText:@"Enter a name for the new folder:"];
        [alert addButtonWithTitle:@"Create"];
        [alert addButtonWithTitle:@"Cancel"];

        NSTextField *input = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 260, 24)];
        [input setStringValue:@"New Folder"];
        [alert setAccessoryView:input];

        NSModalResponse returnCode = [alert runModal];

        if (returnCode == NSAlertFirstButtonReturn) {
            NSString *text = [input stringValue];
            juce::String folderName = juce::String::fromUTF8([text UTF8String]);
            juce::File newFolder = parentFolder.getChildFile(folderName);
            bool success = newFolder.createDirectory().wasOk();
            callback(success);
        } else {
            callback(false);
        }
    });
}

void NativeDialogs::showConfirmation(const juce::String& title, const juce::String& message, std::function<void()> callback)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:[NSString stringWithUTF8String:title.toRawUTF8()]];
        [alert setInformativeText:[NSString stringWithUTF8String:message.toRawUTF8()]];
        [alert addButtonWithTitle:@"OK"];

        [alert runModal];
        callback();
    });
}

void NativeDialogs::showPresetMenu(std::function<void(int)> callback)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        // Create menu handler
        PresetMenuHandler *handler = [[PresetMenuHandler alloc] init];
        handler.callback = ^(int result) {
            callback(result);
        };

        NSMenu *menu = [[NSMenu alloc] initWithTitle:@""];

        NSMenuItem *saveItem = [[NSMenuItem alloc] initWithTitle:@"Save Preset..."
                                                          action:@selector(menuItemSelected:)
                                                   keyEquivalent:@""];
        [saveItem setTag:1];
        [saveItem setTarget:handler];

        NSMenuItem *loadItem = [[NSMenuItem alloc] initWithTitle:@"Load Preset..."
                                                          action:@selector(menuItemSelected:)
                                                   keyEquivalent:@""];
        [loadItem setTag:2];
        [loadItem setTarget:handler];

        [menu addItem:saveItem];
        [menu addItem:loadItem];

        // Get current mouse location in screen coordinates
        NSPoint mouseLocation = [NSEvent mouseLocation];

        // Get the main window
        NSWindow *window = [[NSApplication sharedApplication] mainWindow];
        NSView *view = [window contentView];

        if (window && view) {
            // Convert screen coordinates to window coordinates
            NSPoint windowLocation = [window convertPointFromScreen:mouseLocation];

            // Convert to view coordinates
            NSPoint viewLocation = [view convertPoint:windowLocation fromView:nil];

            // Show menu at location
            [menu popUpMenuPositioningItem:nil atLocation:viewLocation inView:view];
        } else {
            callback(0);  // No window, cancel
        }
    });
}

#else
// Fallback for non-Mac platforms
juce::File NativeDialogs::getPresetsFolder()
{
    juce::File documentsDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
    juce::File presetsFolder = documentsDir.getChildFile("XYControl Presets");
    if (!presetsFolder.exists())
        presetsFolder.createDirectory();
    return presetsFolder;
}

void NativeDialogs::showSaveDialog(const juce::File& presetsFolder, std::function<void(juce::File)> callback)
{
    callback(presetsFolder.getChildFile("my_preset.json"));
}

void NativeDialogs::showPresetBrowser(const juce::File& presetsFolder, std::function<void(juce::File)> onLoad)
{
    onLoad(juce::File());
}

void NativeDialogs::showConfirmation(const juce::String& title, const juce::String& message, std::function<void()> callback)
{
    callback();
}

void NativeDialogs::createNewFolder(const juce::File& parentFolder, std::function<void(bool)> callback)
{
    callback(false);
}

void NativeDialogs::showPresetMenu(std::function<void(int)> callback)
{
    callback(0);
}
#endif

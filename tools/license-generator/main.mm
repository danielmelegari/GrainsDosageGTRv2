#import <Cocoa/Cocoa.h>
#include "ledger.h"
#include "crypto.h"
#include <sys/stat.h>
#include <memory>
using namespace grains::issuer;
@interface Generator : NSObject<NSApplicationDelegate,NSTextFieldDelegate>
@property(strong) NSWindow* window;
@property(strong) NSSegmentedControl* mode;
@property(strong) NSTextField* customer;
@property(strong) NSTextField* license;
@property(strong) NSTextView* machine;
@property(strong) NSTextView* output;
@property(strong) NSTextField* status;
@property(strong) NSButton* generate;
- (void)exportPublic:(id)sender;
- (void)backupPrivate:(id)sender;
@end
@implementation Generator
- (NSTextField*)label:(NSString*)text frame:(NSRect)frame size:(CGFloat)size {
  NSTextField* v=[NSTextField labelWithString:text];v.frame=frame;v.font=[NSFont systemFontOfSize:size];[self.window.contentView addSubview:v];return v;
}
- (NSButton*)button:(NSString*)title action:(SEL)action frame:(NSRect)frame {
  NSButton* b=[NSButton buttonWithTitle:title target:self action:action];b.frame=frame;b.bezelStyle=NSBezelStyleRounded;[self.window.contentView addSubview:b];return b;
}
- (NSTextView*)textArea:(NSRect)frame editable:(BOOL)editable {
  NSScrollView* scroll=[[NSScrollView alloc] initWithFrame:frame];scroll.borderType=NSBezelBorder;scroll.hasVerticalScroller=YES;
  NSTextView* text=[[NSTextView alloc] initWithFrame:NSMakeRect(0,0,frame.size.width-20,frame.size.height)];text.richText=NO;text.editable=editable;text.selectable=YES;text.font=[NSFont userFixedPitchFontOfSize:13];text.textContainerInset=NSMakeSize(10,10);text.automaticQuoteSubstitutionEnabled=NO;text.automaticDashSubstitutionEnabled=NO;text.automaticSpellingCorrectionEnabled=NO;text.verticallyResizable=YES;text.horizontallyResizable=NO;text.autoresizingMask=NSViewWidthSizable;text.textContainer.widthTracksTextView=YES;
  scroll.documentView=text;[self.window.contentView addSubview:scroll];return text;
}
- (void)error:(NSString*)message {NSAlert* alert=[NSAlert new];alert.messageText=@"GrainsDosage License Generator";alert.informativeText=message;[alert runModal];self.status.stringValue=message;}
- (void)applicationDidFinishLaunching:(NSNotification*)notification {
  (void)notification;self.window=[[NSWindow alloc] initWithContentRect:NSMakeRect(0,0,760,730) styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskMiniaturizable backing:NSBackingStoreBuffered defer:NO];self.window.title=[NSString stringWithFormat:@"GrainsDosage License Generator %@",[NSBundle.mainBundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"]?:@""];
  [self label:@"GrainsDosage" frame:NSMakeRect(28,673,470,35) size:28];
  [self label:[NSString stringWithFormat:@"LICENSE GENERATOR v%@  ·  OFFLINE  ·  RSA SIGNED",[NSBundle.mainBundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"]?:@""] frame:NSMakeRect(30,649,700,22) size:12];
  self.mode=[NSSegmentedControl segmentedControlWithLabels:@[@"TRIAL 14 DAYS",@"PERMANENT"] trackingMode:NSSegmentSwitchTrackingSelectOne target:self action:@selector(changeMode:)];self.mode.frame=NSMakeRect(28,598,704,34);self.mode.selectedSegment=0;[self.window.contentView addSubview:self.mode];
  [self label:@"Nome cliente — permanente" frame:NSMakeRect(30,564,690,22) size:13];
  self.customer=[[NSTextField alloc] initWithFrame:NSMakeRect(30,534,700,28)];self.customer.delegate=self;self.customer.placeholderString=@"Usare lo stesso nome per riutilizzare la licenza del cliente";[self.window.contentView addSubview:self.customer];
  [self label:@"License ID — automatico; inserire quello originale per clienti con più licenze" frame:NSMakeRect(30,506,700,22) size:12];
  self.license=[[NSTextField alloc] initWithFrame:NSMakeRect(30,476,700,28)];self.license.placeholderString=@"GDL2-… (opzionale)";[self.window.contentView addSubview:self.license];
  [self label:@"Machine ID completa (GDDEV2-… oppure Windows)" frame:NSMakeRect(30,438,570,24) size:13];
  [self button:@"Incolla" action:@selector(pasteMachine:) frame:NSMakeRect(625,432,110,32)];
  self.machine=[self textArea:NSMakeRect(30,325,700,105) editable:YES];
  self.generate=[self button:@"Genera Trial 14 Giorni" action:@selector(generateLicense:) frame:NSMakeRect(28,277,706,38)];self.generate.keyEquivalent=@"\r";
  [self label:@"Codice completo — selezionabile e copiabile anche dal TXT salvato" frame:NSMakeRect(30,247,690,22) size:12];
  self.output=[self textArea:NSMakeRect(30,126,700,117) editable:NO];
  [self button:@"Copia codice" action:@selector(copyCode:) frame:NSMakeRect(580,86,155,32)];
  [self button:@"Configurazione iniziale…" action:@selector(setup:) frame:NSMakeRect(24,86,198,32)];
  [self button:@"Mostra registro" action:@selector(showLedger:) frame:NSMakeRect(225,86,135,32)];
  [self button:@"Esporta chiave pubblica" action:@selector(exportPublic:) frame:NSMakeRect(365,86,210,32)];
  self.status=[self label:@"" frame:NSMakeRect(30,18,700,60) size:12];self.status.maximumNumberOfLines=3;self.status.lineBreakMode=NSLineBreakByWordWrapping;
  [self changeMode:nil];[self.window center];[self.window makeKeyAndOrderFront:nil];[NSApp activateIgnoringOtherApps:YES];
  try{
    NSString* path=ledgerPath();
    SecKeyRef ref=loadKey();std::unique_ptr<std::remove_pointer_t<SecKeyRef>,decltype(&CFRelease)> key(ref,&CFRelease);
    self.status.stringValue=path?@"Configurato. Il registro conserva i dispositivi e le date trial già emesse.":
      matches(ref)?@"La tua chiave è riconosciuta. Premi Genera oppure Configurazione iniziale per configurare il registro. Non creare un’altra chiave.":
      ref?@"Chiave già salvata: esporta solo il file pubblico e installa le build abbinate. Non creare un’altra chiave.":
      @"Prima configurazione: importa la tua chiave oppure creala su questo Mac. Non serve possedere già un PEM.";
  }catch(const std::exception& e){[self error:@(e.what())];}
}
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender{(void)sender;return YES;}
- (void)controlTextDidChange:(NSNotification*)note{if(note.object==self.customer)self.license.stringValue=@"";}
- (void)changeMode:(id)sender{(void)sender;BOOL permanent=self.mode.selectedSegment==1;self.customer.enabled=permanent;self.license.enabled=permanent;self.generate.title=permanent?@"Genera Licenza Permanente":@"Genera Trial 14 Giorni";self.output.string=@"";}
- (void)pasteMachine:(id)sender{(void)sender;NSString* s=[[NSPasteboard generalPasteboard] stringForType:NSPasteboardTypeString];if(s)self.machine.string=s;}
- (void)copyCode:(id)sender{(void)sender;if(!self.output.string.length)return;NSPasteboard* p=[NSPasteboard generalPasteboard];[p clearContents];[p setString:self.output.string forType:NSPasteboardTypeString];}
- (void)showLedger:(id)sender{(void)sender;try{NSString* path=ledgerPath();if(path)[[NSWorkspace sharedWorkspace] activateFileViewerSelectingURLs:@[[NSURL fileURLWithPath:path]]];else [self error:@"Configurare prima il registro."];}catch(const std::exception& e){[self error:@(e.what())];}}
- (void)exportPublic:(id)sender {
  (void)sender;try{
    SecKeyRef ref=loadKey();std::unique_ptr<std::remove_pointer_t<SecKeyRef>,decltype(&CFRelease)> key(ref,&CFRelease);
    if(!ref)throw std::runtime_error("Premere Configurazione iniziale e creare o importare la chiave.");
    NSSavePanel* panel=[NSSavePanel savePanel];panel.title=@"Esporta SOLO la chiave pubblica";panel.nameFieldStringValue=@"GrainsDosage-Public-Key.json";
    panel.message=@"Questo file pubblico può essere inviato per aggiornare plugin e generatore. Non contiene la chiave privata. Le build precedenti non accetteranno le licenze della nuova chiave.";
    if([panel runModal]!=NSModalResponseOK)return;
    if(![publicEnrollment(ref) writeToURL:panel.URL options:NSDataWritingAtomic error:nil])throw std::runtime_error("Salvataggio della chiave pubblica fallito. Riprovare senza creare un'altra chiave.");
    self.status.stringValue=@"Invia GrainsDosage-Public-Key.json per le build abbinate. Conserva questa chiave nel Portachiavi: non rigenerarla. Dopo l’aggiornamento, riapri Configurazione iniziale.";
  }catch(const std::exception& e){[self error:@(e.what())];}
}
- (void)backupPrivate:(id)sender {
  (void)sender;try{
    SecKeyRef ref=loadKey();std::unique_ptr<std::remove_pointer_t<SecKeyRef>,decltype(&CFRelease)> key(ref,&CFRelease);
    if(!ref)throw std::runtime_error("Creare o importare prima la chiave.");
    NSSavePanel* panel=[NSSavePanel savePanel];panel.title=@"Backup PRIVATO cifrato — non inviare";panel.nameFieldStringValue=@"GrainsDosage-Private-Key-Backup.pem";
    panel.message=@"Salva su un supporto sicuro. Non caricare questo PEM in chat o su GitHub e non inviarlo ai clienti. macOS chiederà una password per cifrarlo.";
    if([panel runModal]!=NSModalResponseOK)return;
    NSData* backup=encryptedBackup(ref);
    if(![backup writeToURL:panel.URL options:NSDataWritingAtomic error:nil])throw std::runtime_error("Backup non salvato. La chiave rimane nel Portachiavi.");
    if(chmod(panel.URL.path.fileSystemRepresentation,0600)!=0)throw std::runtime_error("Backup salvato, ma impossibile impostare i permessi. Spostarlo in un archivio privato.");
    self.status.stringValue=@"Backup PEM cifrato salvato. Conserva la password separatamente. Condividi esclusivamente il file Public-Key.json.";
  }catch(const std::exception& e){[self error:@(e.what())];}
}
- (void)setup:(id)sender {
  (void)sender;
  try{
    if(ledgerPath()){[self error:@"Configurazione già presente. La chiave non viene sostituita. Se il registro manca, ripristinare il backup nel percorso originale (Mostra registro). Conservare un solo registro autorevole."];return;}
    SecKeyRef existing=loadKey();std::unique_ptr<std::remove_pointer_t<SecKeyRef>,decltype(&CFRelease)> key(existing,&CFRelease);
    if(!existing){
      NSAlert* start=[NSAlert new];start.messageText=@"Configura la tua chiave di firma";start.informativeText=@"Se non hai mai avuto la chiave privata, puoi crearla su questo Mac. Verrà conservata nel Portachiavi. Poi invia SOLO il file pubblico per ricevere plugin e generatore abbinati: le build attuali non conoscono la nuova chiave. Se hai già un PEM originale, importalo.";
      [start addButtonWithTitle:@"Crea la mia chiave su questo Mac"];[start addButtonWithTitle:@"Importa PEM esistente…"];[start addButtonWithTitle:@"Annulla"];
      NSModalResponse first=[start runModal];if(first==NSAlertThirdButtonReturn)return;
      if(first==NSAlertFirstButtonReturn){key.reset(createPublisherKey());[self exportPublic:nil];return;}
      NSOpenPanel* panel=[NSOpenPanel openPanel];panel.title=@"Importa la chiave privata RSA originale";panel.message=@"Seleziona il PEM del produttore corrispondente alla chiave pubblica incorporata nel plugin. Il PEM non si trova nel plugin o nel pacchetto scaricato. Verrà importato nel Portachiavi una sola volta.";panel.canChooseDirectories=NO;panel.allowsMultipleSelection=NO;
      if([panel runModal]!=NSModalResponseOK)return;
      NSNumber* size=nil;[panel.URL getResourceValue:&size forKey:NSURLFileSizeKey error:nil];if(size.unsignedLongLongValue>65536)throw std::runtime_error("File chiave troppo grande.");
      NSData* data=[NSData dataWithContentsOfURL:panel.URL];if(!data)throw std::runtime_error("Impossibile leggere la chiave privata.");key.reset(importKey(data));
    }
    if(!matches(key.get())){[self error:@"La chiave è già custodita nel Portachiavi. Per emettere licenze servono le build abbinate alla sua chiave pubblica. Esporta e invia SOLO GrainsDosage-Public-Key.json; non creare un’altra chiave."];[self exportPublic:nil];return;}
    const bool firstIssuance=!existing||freshKey(key.get());
    NSAlert* choice=[NSAlert new];choice.messageText=@"Registro delle licenze";choice.informativeText=@"Se sono già state emesse licenze, usare il registro SQLite originale per conservare il limite di 3 dispositivi. Usare Nuovo registro SOLO se non esistono emissioni precedenti.";[choice addButtonWithTitle:@"Usa registro esistente…"];if(firstIssuance)[choice addButtonWithTitle:@"Nuovo registro (prima emissione)"];[choice addButtonWithTitle:@"Annulla"];
    NSModalResponse response=[choice runModal];BOOL create=firstIssuance&&response==NSAlertSecondButtonReturn;if(response!=NSAlertFirstButtonReturn&&!create)return;
    NSString* path=nil;
    if(create){NSString* root=[NSHomeDirectory() stringByAppendingPathComponent:@"Library/Application Support/GrainsDosage License Generator"];NSError* error=nil;if(![[NSFileManager defaultManager] createDirectoryAtPath:root withIntermediateDirectories:YES attributes:@{NSFilePosixPermissions:@0700} error:&error])throw std::runtime_error("Impossibile creare la cartella del registro.");path=[root stringByAppendingPathComponent:@"licenses.sqlite"];if([[NSFileManager defaultManager] fileExistsAtPath:path])throw std::runtime_error("Un registro esiste già. Scegliere Usa registro esistente; non verrà sovrascritto.");}
    else{NSOpenPanel* p=[NSOpenPanel openPanel];p.title=@"Seleziona il registro SQLite originale";p.canChooseDirectories=NO;p.allowsMultipleSelection=NO;if([p runModal]!=NSModalResponseOK)return;path=p.URL.path;}
    {Ledger db(path.fileSystemRepresentation,fingerprint(),create);}
    if(chmod(path.fileSystemRepresentation,0600)!=0)throw std::runtime_error("Impossibile proteggere i permessi del registro.");
    if(!existing)storeKey(key.get());storeLedgerPath(path);finishEnrollment();
    self.status.stringValue=@"Configurazione completata. Chiave nel Portachiavi; registro persistente. Eseguire backup sicuri di entrambi.";
  }catch(const std::exception& e){[self error:@(e.what())];}
}
- (void)generateLicense:(id)sender {
  (void)sender;self.output.string=@"";
  try{
    NSString* path=ledgerPath();
    // A paired first-time publisher can complete the ledger setup directly
    // from Generate, instead of seeing the old circular import/create message.
    if(!path){[self setup:nil];path=ledgerPath();if(!path)return;}
    SecKeyRef ref=loadKey();std::unique_ptr<std::remove_pointer_t<SecKeyRef>,decltype(&CFRelease)> key(ref,&CFRelease);if(!ref)throw std::runtime_error("Chiave assente nel Portachiavi. Ripristinare il backup della chiave originale.");
    NSString* machine=[self.machine.string stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
    // Windows registration requests can be pasted in full as JSON, too.
    if([machine hasPrefix:@"{"]){id object=[NSJSONSerialization JSONObjectWithData:[machine dataUsingEncoding:NSUTF8StringEncoding] options:0 error:nil];if([object isKindOfClass:NSDictionary.class]&&[object[@"machine_id"] isKindOfClass:NSString.class])machine=object[@"machine_id"];}
    std::string device(machine.UTF8String);if(!deviceValid(device))throw std::runtime_error("Machine ID non valida o incompleta. Incollare GDDEV2- + 64 caratteri oppure la Machine ID Windows di 64 caratteri.");
    BOOL trial=self.mode.selectedSegment==0;if(!trial&&customerKey(self.customer.stringValue.UTF8String).empty())throw std::runtime_error("Inserire il nome cliente.");NSString* filename=trial?@"GrainsDosage-Trial-Key.txt":@"GrainsDosage-Permanent-Key.txt";
    // Choose a directory so the exported filename is always exactly the requested one.
    NSOpenPanel* save=[NSOpenPanel openPanel];save.title=[@"Salva " stringByAppendingString:filename];save.message=@"Scegli la cartella del cliente. Verrà salvato unicamente il file TXT indicato.";save.canChooseFiles=NO;save.canChooseDirectories=YES;save.canCreateDirectories=YES;save.allowsMultipleSelection=NO;save.prompt=@"Salva qui";if([save runModal]!=NSModalResponseOK)return;
    NSURL* output=[save.URL URLByAppendingPathComponent:filename];if([[NSFileManager defaultManager] fileExistsAtPath:output.path]){NSAlert* a=[NSAlert new];a.messageText=@"Sostituire il TXT già presente?";a.informativeText=output.path;[a addButtonWithTitle:@"Sostituisci"];[a addButtonWithTitle:@"Annulla"];if([a runModal]!=NSAlertFirstButtonReturn)return;}
    Ledger db(path.fileSystemRepresentation,fingerprint());Grant grant;
    NSString* normalized=[[self.customer.stringValue stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet] precomposedStringWithCanonicalMapping];
    NSString* requested=[self.license.stringValue stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
    NSString* fresh=[@"GDL2-" stringByAppendingString:[[NSUUID.UUID.UUIDString stringByReplacingOccurrencesOfString:@"-" withString:@""] lowercaseString]];
    const auto code=db.issue(trial,device,normalized.UTF8String,requested.UTF8String,fresh.UTF8String,(int64_t)[NSDate.date timeIntervalSince1970],[&](const Grant& g){return signPayload(key.get(),g.payload,macDevice(device));},grant);
    NSString* text=[[NSString alloc] initWithBytes:code.data() length:code.size() encoding:NSUTF8StringEncoding];NSError* error=nil;
    if(![text writeToURL:output atomically:YES encoding:NSUTF8StringEncoding error:&error])throw std::runtime_error("Impossibile salvare il TXT. Il registro è già aggiornato: riprovare con lo stesso cliente e dispositivo, senza consumare un altro posto.");
    self.output.string=text;
    if(trial){NSDateFormatter* f=[NSDateFormatter new];f.dateFormat=@"yyyy-MM-dd HH:mm:ss 'UTC'";f.timeZone=[NSTimeZone timeZoneForSecondsFromGMT:0];self.status.stringValue=[NSString stringWithFormat:@"Salvato %@\nInizio: %@ · Scadenza: %@. Riemettere il codice non rinnova il trial.",filename,[f stringFromDate:[NSDate dateWithTimeIntervalSince1970:grant.start]],[f stringFromDate:[NSDate dateWithTimeIntervalSince1970:grant.end]]];}
    else{self.license.stringValue=@(grant.license.c_str());self.status.stringValue=[NSString stringWithFormat:@"Salvato %@\n%@ · %d/3 dispositivi registrati.",filename,self.license.stringValue,grant.devices];}
  }catch(const std::exception& e){[self error:@(e.what())];}
}
@end
int main(int argc,const char** argv){(void)argc;(void)argv;umask(0077);@autoreleasepool{[NSApplication sharedApplication];[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
  NSMenu* menu=[NSMenu new];NSMenuItem* root=[NSMenuItem new];[menu addItem:root];NSMenu* app=[NSMenu new];[app addItemWithTitle:@"Esci da GrainsDosage License Generator" action:@selector(terminate:) keyEquivalent:@"q"];root.submenu=app;
  NSMenuItem* editRoot=[NSMenuItem new];[menu addItem:editRoot];NSMenu* edit=[[NSMenu alloc] initWithTitle:@"Modifica"];[edit addItemWithTitle:@"Taglia" action:@selector(cut:) keyEquivalent:@"x"];[edit addItemWithTitle:@"Copia" action:@selector(copy:) keyEquivalent:@"c"];[edit addItemWithTitle:@"Incolla" action:@selector(paste:) keyEquivalent:@"v"];[edit addItemWithTitle:@"Seleziona tutto" action:@selector(selectAll:) keyEquivalent:@"a"];editRoot.submenu=edit;NSApp.mainMenu=menu;
  Generator* delegate=[Generator new];NSMenuItem* backup=[app insertItemWithTitle:@"Backup chiave privata cifrato…" action:@selector(backupPrivate:) keyEquivalent:@"" atIndex:0];backup.target=delegate;NSApp.delegate=delegate;[NSApp run];}return 0;}

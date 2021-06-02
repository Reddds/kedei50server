/*
Использование:

Controls.init('/tmp/kedei_lcd_in', '/tmp/kedei_lcd_out');

const clearAllControls = new Controls.ClearAllControls();
        clearAllControls.send();



*/


import * as hexdump from 'hexdump-nodejs';
import * as fs from 'fs';

var AsyncLock = require('async-lock');
var lock = new AsyncLock();

let wstreamWrite: fs.WriteStream;// = fs.createWriteStream('/tmp/kedei_lcd_in');
let wstreamRead: fs.ReadStream;// = fs.createReadStream('/tmp/kedei_lcd_out');

let AllControls = Array<Control>();

var GlobalEventCounter = 1;

export function init(outStreamPath: string, inStreamPath: string) {
    const eventReseiveHeaderLen = 12;
    const readTimeoutMs = 5000;

    /**
     * 0 - idle or waiting event header
     * 1 - waiting event header
     * 2 - waiting additional
     */
    let readEventStage = 0;
    let curEvent: EventData;
    let addDataLen: number;
    let startTime: number;

    wstreamRead = fs.createReadStream(inStreamPath);



    wstreamWrite = fs.createWriteStream(outStreamPath);


    wstreamRead.on('readable', () => {
        //console.log('readable paused = ' + wstreamRead.isPaused());

        switch (readEventStage) {
            case 0:
                // console.log("readEventStage = 0!");
                this.startTime = (new Date).getTime();
            case 1:
                {
                    // console.log("readEventStage = 1!");
                    let rrr = wstreamRead.read(eventReseiveHeaderLen);
                    if (rrr === null) {
                        let curMs = (new Date).getTime();
                        console.log("startMs = " + this.startTime + ", curMs = " + curMs + ", diff = " + (curMs - this.startTime));
                        if ((curMs - this.startTime) > readTimeoutMs) {
                            console.log("Timeout!");
                            let rd = wstreamRead.read();
                            if (rd !== null) {
                                console.log("Left in stream: " + rd.length);
                                console.log(hexdump(rd));
                            }
                            readEventStage = 0;
                        }
                        return;
                    }
                    let buffer = new ArrayBuffer(eventReseiveHeaderLen);
                    var uint8 = new Uint8Array(buffer);
                    //console.log("Read " + rrr.length + " bytes");
                    //console.log(hexdump(rrr));
                    var uint8 = new Uint8Array(buffer);
                    uint8.set(rrr, 0);
                    var vId = new DataView(buffer);
                    let namebuf = buffer.slice(6, 10);

                    this.addDataLen = vId.getUint16(10, true);

                    // curEvent.eventId = vId.getUint32(0, true);
                    // curEvent.controlId = vId.getUint16(4, true);
                    // //console.log("Buffer.from(buffer, 6, 4)");
                    // //console.log(hexdump(namebuf));
                    // curEvent.name = Buffer.from(namebuf).toString('ascii');
                    // curEvent.additional = null;

                    curEvent = new EventData(vId.getUint32(0, true),
                        vId.getUint16(4, true),
                        Buffer.from(namebuf).toString('ascii'),
                        null);


                    if (this.addDataLen == 0) {
                        readEventStage = 0;

                        processEvent(curEvent);
                        return;
                    }
                    else {
                        this.startTime = (new Date).getTime();
                        readEventStage = 2;
                        return;
                    }
                }

            case 2:
                {
                    console.log("readEventStage = 2! waiting for " + this.addDataLen + " bytes...");
                    let addRrr = wstreamRead.read(this.addDataLen);
                    if (addRrr === null) {

                        let curMs = (new Date).getTime();
                        console.log("startMs = " + this.startTime + ", curMs = " + curMs + ", diff = " + (curMs - this.startTime));
                        if ((curMs - this.startTime) > readTimeoutMs) {
                            console.log("Timeout!");
                            let rd = wstreamRead.read();
                            if (rd !== null) {
                                console.log("Left in stream: " + rd.length);
                                console.log(hexdump(rd));
                            }
                            readEventStage = 0;
                            return;
                        }
                        return;
                    }
                    let addBuffer = new ArrayBuffer(this.addDataLen);

                    var addUint8 = new Uint8Array(addBuffer);


                    /*if(addRrr == null)
                    {
                        console.log("Read additional data = null");
                    }
                    else
                    {
                        console.log("Read additional data " + addRrr.length);
                    }*/
                    addUint8.set(addRrr, 0);
                    curEvent.additional = addBuffer;
                    readEventStage = 0;
                    processEvent(curEvent);
                }
                break;
        }
    });
}

export function EndWrite() {
    if (wstreamWrite) {

        console.log("End writing", wstreamWrite.path);
        wstreamWrite.end();
    }
}


export class EventData {
    public eventId: number;
    public controlId: number;
    public name: string;
    public additional: ArrayBuffer;

    constructor(eventId: number, controlId: number, name: string, additional: ArrayBuffer) {
        this.eventId = eventId;
        this.controlId = controlId;
        this.name = name;
        this.additional = additional;
    }

    public dump() {
        let addBytes = 0;
        if (this.additional !== null) {
            addBytes = this.additional.byteLength;
        }

        console.log("EventData: eventId = " + this.eventId + ", controlId = " + this.controlId + ", name = '" + this.name + "', additional = " + addBytes);
    }
}

class TouchEventData extends EventData {
    private _x: number;
    private _y: number;
    constructor(eventData: EventData) {
        super(eventData.eventId, eventData.controlId, eventData.name, eventData.additional);

        if (eventData.additional == null || eventData.additional.byteLength < 4) {
            new Error("Touch event additional length error!");
            return;
        }

        var vId = new DataView(eventData.additional);
        this._x = vId.getUint16(0, true);
        this._y = vId.getUint16(2, true);
        console.log("Touch x = " + this._x + ", y = " + this._y);

    }

    public get x() {
        return this._x;
    }

    public get y() {
        return this._y;
    }
}

export function processEvent(curEvent: EventData) {
    //curEvent.dump();

    if (curEvent.name == 'crok') // touch
    {
        // console.log("Skipping crok event");
        return;
    }

    let processedEvent: EventData;
    if (curEvent.name == 'toch') // touch
    {
        processedEvent = new TouchEventData(curEvent);

    }

    if (processedEvent == null) {
        console.error("Error processing event!", curEvent.name);
        return;
    }

    if (curEvent.controlId == 0) {
        return;
    }

    let control = AllControls[curEvent.controlId];

    if (control == null) {
        console.error("Control id " + curEvent.controlId + " not found!");
        return;
    }

    control.doEvent(processedEvent);
}

function getGlobalEventId(): number {
    let retVal;
    // Promise mode
    lock.acquire(GlobalEventCounter, function () {
        // return value or promise
        GlobalEventCounter++;
        retVal = GlobalEventCounter;

    }).then(function () {
        // lock released
    });
    return retVal;
}

function strToBytes(str) {
    var arr = [];
    for (var i = 0; i < str.length; i++) {
        var charcode = str.charCodeAt(i);
        if (charcode < 0x80)
            arr.push(charcode);
    }

    return Uint8Array.from(arr);
}

class Command {
    //private _wstreamWrite : fs.WriteStream;
    private _eventId: number;
    /*constructor(wstreamWrite: fs.WriteStream)
    {
        this._wstreamWrite = wstreamWrite;
    }*/

    protected getCommandBytes() {
        console.log("Command getCreateBytes");
        return null;
    }

    public get eventId() {
        return this._eventId;
    }

    protected sendBytes(event_id: number, bytes: ArrayBuffer) {
        var buffer = new ArrayBuffer(4);
        var vId = new DataView(buffer);
        vId.setUint32(0, event_id, true);
        var buff = Buffer.from(buffer);
        console.log("sending command id = " + event_id + " ... ");
        wstreamWrite.write(buff);
        if (bytes == null) {
            console.error("Bytes - null!");
            return;
        }
        buff = Buffer.from(bytes);
        console.log(hexdump(buff));
        wstreamWrite.write(buff);
    }

    protected sendEvent(bytes: ArrayBuffer) {
        this._eventId = getGlobalEventId();
        this.sendBytes(this._eventId, bytes);
        return this._eventId;
    }

    public send() {
        this.sendEvent(this.getCommandBytes());
    }
}

export class ClearAllControls extends Command {
    protected getCommandBytes() {
        var buffer = new ArrayBuffer(4);
        var uint8 = new Uint8Array(buffer);
        uint8.set(strToBytes("ddac"));
        return buffer;
    }
}

export enum date_time_comb_tag {
    DT_COMB_NONE = 0,
    DT_COMB_ONLY_TIME = 1,
    // time in time_id, date in date_id
    DT_COMB_TIME_AND_DATE = 2,
    DT_COMB_ONLY_DATE = 3,
    DT_COMB_TIME_BEFORE_DATE = 4,
    DT_COMB_DATE_BEFORE_TIME = 5
};

export enum date_time_time_fmt_tag {
    //  5:05:46 PM
    DT_TM_II_0MM_0SS = 1,
    //  17:05:46
    DT_TM_HH_0MM_0SS = 2,

    //  5:05 PM
    DT_TM_II_0MM = 10,
    //  17:05
    DT_TM_HH_0MM = 11
};

export enum date_time_date_fmt_tag {
    /**
     * "12.04.2018"
     */
    DT_DT_DD_MM_YYYY = 1,
    // 2 	"12.04.18"
    DT_DT_DD_MM_YY = 2,

    // 10    "12 апр 2018"
    DT_DT_DD_MMM_YYYY = 10,
    // 11    "12 апр 18"
    DT_DT_DD_MMM_YY = 11,
    // 12    "12 апреля 2018"
    DT_DT_DD_MMMM_YYYY = 12,
    // 13    "12 апреля 18"
    DT_DT_DD_MMMM_YY = 13,

    /**
     * "12 апр"
     */
    DT_DT_DD_MMM = 20,
    /**
     * "12 апреля"
     */
    DT_DT_DD_MMMM = 21,

    // 30    "Четв. 12.04.2018"
    DT_DT_WWW_DD_MM_YYYY = 30,
    // 31    "Четв. 12.04.18"
    DT_DT_WWW_DD_MM_YY = 31,
    // 32    "Четв. 12 апр 2018"
    DT_DT_WWW_DD_MMM_YYYY = 32,
    // 33    "Четв. 12 апр 18"
    DT_DT_WWW_DD_MMM_YY = 33,
    // 34    "Четв. 12 апреля 2018"
    DT_DT_WWW_DD_MMMM_YYYY = 34,
    // 35    "Четв. 12 апреля 18"
    DT_DT_WWW_DD_MMMM_YY = 35,

    /**
     * "Четв. 12 апр"
     */
    DT_DT_WWW_DD_MMM = 36,
    /**
     * "Четв. 12 апреля"
     */
    DT_DT_WWW_DD_MMMM = 37,


    // 38    "Четверг, 12.04.2018"
    DT_DT_WWWW_DD_MM_YYYY = 38,
    // 39    "Четверг, 12.04.18"
    DT_DT_WWWW_DD_MM_YY = 39,
    // 40    "Четверг, 12 апр 2018"
    DT_DT_WWWW_DD_MMM_YYYY = 40,
    // 41    "Четверг, 12 апр 18"
    DT_DT_WWWW_DD_MMM_YY = 41,
    // 42    "Четверг, 12 апреля 2018"
    DT_DT_WWWW_DD_MMMM_YYYY = 42,
    // 43    "Четверг, 12 апреля 18"
    DT_DT_WWWW_DD_MMMM_YY = 43,

    /**
     *  "Четверг, 12 апр"
     */
    DT_DT_WWWW_DD_MMM = 44,
    /**
     * "Четверг, 12 апреля"
     */
    DT_DT_WWWW_DD_MMMM = 45,

};

export class ConfigDateAndTime extends Command {

    private _timeLabel: Label;
    private _dateLabel: Label;
    private _dtCombine: date_time_comb_tag;
    private _timeFormat: date_time_time_fmt_tag;
    private _dateFormat: date_time_date_fmt_tag;
    constructor(timeLabel: Label, dateLabel: Label,
        dtCombine: date_time_comb_tag, timeFormat: date_time_time_fmt_tag, dateFormat: date_time_date_fmt_tag) {
        super();

        this._timeLabel = timeLabel;
        this._dateLabel = dateLabel;
        this._dtCombine = dtCombine;
        this._timeFormat = timeFormat;
        this._dateFormat = dateFormat;
    }

    protected getCommandBytes() {
        let timeLabelId = 0;
        let dateLabelId = 0;
        if (this._timeLabel != null) {
            timeLabelId = this._timeLabel.id;
        }
        if (this._dateLabel != null) {
            dateLabelId = this._dateLabel.id;
        }
        var buffer = new ArrayBuffer(11);
        var uint8 = new Uint8Array(buffer);
        uint8.set(strToBytes("dstc"));
        var vId = new DataView(buffer);
        vId.setUint16(4, timeLabelId, true);
        vId.setUint16(6, dateLabelId, true);
        vId.setUint8(8, this._dtCombine);
        vId.setUint8(9, this._timeFormat);
        vId.setUint8(10, this._dateFormat);
        return buffer;
    }
}


class Control extends Command {
    protected _parent: ContainerControl;
    protected _id: number;
    protected _x: number;
    protected _y: number;
    protected _width: number;
    protected _height: number;
    protected _visible: boolean;

    protected _onClick: Function;

    constructor(id: number, parent: ContainerControl, x: number, y: number, width: number, height: number, visible: boolean) {
        super();
        this._parent = parent;
        this._id = id;
        this._x = x;
        this._y = y;
        this._width = width;
        this._height = height;
        this._visible = visible;

        if (AllControls[id] != null) {
            new Error("Control with this id (" + id + ") already exists!");
        }
        else {
            AllControls[id] = this;
        }
    }

    protected getCreateBytesHeader(commandName: string): Uint8Array {

        console.log("draw", commandName);

        let parentId = 0;
        if (this._parent != null)
            parentId = this._parent.id;

        if (commandName.length != 4)
            return null;
        var buffer = new ArrayBuffer(17);
        var uint8 = new Uint8Array(buffer);
        uint8.set(strToBytes(commandName));
        var vId = new DataView(buffer);
        vId.setUint16(4, this._id, true);
        vId.setUint16(6, parentId, true);
        vId.setUint16(8, this._x, true);
        vId.setUint16(10, this._y, true);
        vId.setUint16(12, this._width, true);
        vId.setUint16(14, this._height, true);
        vId.setUint8(16, 1); // visible
        return uint8;
    }

    set onClick(fun: Function) {
        this._onClick = fun;
    }

    public doEvent(ed: EventData) {
        console.log("doEvent");
        let tochEv = ed as TouchEventData;
        //console.log(" ed as TouchEventData = " + tochEv);
        if (tochEv != null) {
            if (this._onClick != null)
                this._onClick(tochEv.x, tochEv.y);
        }
    }

    get id(): number {
        return this._id;
    }

    protected getCreateBytes() {
        console.log("def getCreateBytes");
        return null;
    }

    protected show() {
        this.sendEvent(this.getCreateBytes());
    }

    // name - message name
    // payload - additional data
    public onMessage(name: string, payload) {

    }

    get visible() {
        return this._visible;
    }

    set visible(vis: boolean) {
        if (this._visible == vis)
            return;
        this._visible = vis;
        var buffer = new ArrayBuffer(7);
        var uint8 = new Uint8Array(buffer);
        uint8.set(strToBytes("dsvi"));
        var vId = new DataView(buffer);
        vId.setUint16(4, this._id, true);
        vId.setUint8(6, this._visible ? 1 : 0);
        this.sendEvent(buffer);

    }
}

class ContainerControl extends Control {


}

export class Panel extends ContainerControl {
    private _bgR: number;
    private _bgG: number;
    private _bgB: number;

    constructor(id: number, parent: ContainerControl, x: number, y: number, width: number, height: number, visible: boolean,
        r: number = 0, g: number = 0, b: number = 0) {
        super(id, parent, x, y, width, height, visible);
        this._bgR = r;
        this._bgG = g;
        this._bgB = b;

        this.show();
    }

    protected getCreateBytes() {
        let header = this.getCreateBytesHeader("dpan");

        var buffer = new ArrayBuffer(header.length + 3);
        var uint8 = new Uint8Array(buffer);
        uint8.set(header);
        var vId = new DataView(buffer);
        vId.setUint8(17, this._bgR);
        vId.setUint8(18, this._bgG);
        vId.setUint8(19, this._bgB);

        console.log("Panel getCreateBytes " + buffer.byteLength);
        return buffer;
    }

}

export class TextControl extends Control {
    protected _text: string;

    get text() {
        return this._text;
    }

    //utf8 to 1251 converter (1 byte format, RU/EN support only + any other symbols) by drgluck
    private utf8_decode(aa: string): Uint8Array {
        const bb = [];
        let c = 0;
        for (var i = 0; i < aa.length; i++) {
            c = aa.charCodeAt(i);
            if (c > 127) {
                if (c > 1024) {
                    if (c == 1025) {
                        c = 1016;
                    } else if (c == 1105) {
                        c = 1032;
                    }
                    bb.push(c - 848);
                }
            } else {
                bb.push(c);
            }
        }
        return Uint8Array.from(bb);
    }

    //dstx\x01\x00\x05\x00World
    set text(txt: string) {
        this._text = txt;
        var enc = new TextEncoder();
        const textBuf = enc.encode(txt);
        // const textBuf = this.utf8_decode(txt);

        const textLen = textBuf.length;// this._text.length;
        var arrlen = 8 + textLen;
        var buffer = new ArrayBuffer(arrlen);
        var uint8 = new Uint8Array(buffer);
        uint8.set(strToBytes("dstx"));
        var vId = new DataView(buffer);
        vId.setUint16(4, this._id, true);
        vId.setUint16(6, textLen, true);
        // uint8.set(strToBytes(this._text), 8);
        uint8.set(textBuf, 8);
        this.sendEvent(buffer);
    }
}

export class TextBox extends TextControl {
    private _fontSize: number;
    private _r: number;
    private _g: number;
    private _b: number;
    constructor(id: number, parent: ContainerControl, x: number, y: number, width: number, height: number, visible: boolean, text = "",
        fsize: number = 32, r: number = 0, g: number = 0, b: number = 0) {
        super(id, parent, x, y, width, height, visible);
        this._text = text;
        this._fontSize = fsize;
        this._r = r;
        this._g = g;
        this._b = b;

        this.show();
    }

    protected getCreateBytes() {
        let header = this.getCreateBytesHeader("dtbx");

        const textLen = this._text.length;

        var buffer = new ArrayBuffer(header.length + 7 + textLen);
        var uint8 = new Uint8Array(buffer);
        uint8.set(header);
        var vId = new DataView(buffer);

        vId.setUint16(17, this._fontSize, true);
        vId.setUint8(19, this._r);
        vId.setUint8(20, this._g);
        vId.setUint8(21, this._b);
        vId.setUint16(22, textLen, true);
        uint8.set(strToBytes(this._text), 24);
        return buffer;
    }
}

export enum TextAlingment {
    TA_LEFT_TOP,
    TA_CENTER_TOP,
    TA_RIGHT_TOP,
    TA_LEFT_MIDDLE, // default
    TA_CENTER_MIDDLE,
    TA_RIGHT_MIDDLE,
    TA_LEFT_BOTTOM,
    TA_CENTER_BOTTOM,
    TA_RIGHT_BOTTOM
}

export class Label extends TextControl {
    private _fontSize: number;
    private _r: number;
    private _g: number;
    private _b: number;
    private _textAling: TextAlingment
    constructor(id: number, parent: ContainerControl, x: number, y: number, width: number, height: number, visible: boolean, text = "",
        fsize: number = 32, r: number = 0, g: number = 0, b: number = 0, textAling: TextAlingment) {
        super(id, parent, x, y, width, height, visible);
        this._text = text;
        this._fontSize = fsize;
        this._r = r;
        this._g = g;
        this._b = b;
        this._textAling = textAling;

        this.show();
    }

    protected getCreateBytes() {
        let header = this.getCreateBytesHeader("dlbl");

        const textLen = this._text.length;

        var buffer = new ArrayBuffer(header.length + 8 + textLen);
        var uint8 = new Uint8Array(buffer);
        uint8.set(header);
        var vId = new DataView(buffer);

        vId.setUint16(17, this._fontSize, true);
        vId.setUint8(19, this._r);
        vId.setUint8(20, this._g);
        vId.setUint8(21, this._b);
        vId.setUint8(22, this._textAling);

        vId.setUint16(23, textLen, true);
        uint8.set(strToBytes(this._text), 25);
        return buffer;
    }
}

export enum DkImageTypes {
    Png = 0
}

export enum DkImageScaleTypes {
    FitWidth = 1,
    FitHeight = 2,
    FitOnMaxDimention = 3,
    FitOnMinDimension = 4,
    Stretch = 5
}

export class Image extends Control {
    private _bgR: number;
    private _bgG: number;
    private _bgB: number;
    private _imageType: DkImageTypes;
    private _scaleType: DkImageScaleTypes;
    private _imageFilePath: string;
    private _imageUrl: string;
    //private _imageData: Buffer;

    constructor(id: number, parent: ContainerControl, x: number, y: number, width: number, height: number, visible: boolean,
        imageType: DkImageTypes, scaleType: DkImageScaleTypes, bgR: number, bgG: number, bgB: number, imageFilePath: string, imageUrl: string) {
        super(id, parent, x, y, width, height, visible);
        this._imageType = imageType;
        this._scaleType = scaleType;
        this._bgR = bgR;
        this._bgG = bgG;
        this._bgB = bgB;
        this._imageFilePath = imageFilePath;
        this._imageUrl = imageUrl;


        if (!fs.existsSync(this._imageFilePath)) {
            console.error("Image file not existis!");
            throw new Error("Image file not existis!");
        }

        this.show();

        // костыль
        this.sendImage();
    }

    private downloadImage(imageUrl: string, callback) {
        var request = require('request').defaults({ encoding: null });
        request(imageUrl, function (error, response, body) {
            if (error != null)
                console.log('error:', error); // Print the error if one occurred
            //console.log('statusCode:', response && response.statusCode); // Print the response status code if a response was received
            //console.log('body:', body); // Print the HTML for the Google homepage.
            //            if(body != null)
            //                this._imageData = body;
            callback(body);
        });
    }

    private sendImage() {
        if (this._imageFilePath != null) {
            console.log("Reading image file " + this._imageFilePath);
            try {
                const imgData = fs.readFileSync(this._imageFilePath);
                console.log("wrighting image data " + imgData.length);
                wstreamWrite.write(imgData);
            } catch (error) {
                console.log("There was an error sending the image", error)
            }

            // fs.readFile(this._imageFilePath, function (err, data) {
            //     console.log("wrighting image data " + data.length);
            //     if (err) {
            //         console.log("There was an error writing the image")
            //     }

            //     wstreamWrite.write(data);
            // });
            return;
        }

        /*if(this._imageData != null)
        {
            wstreamWrite.write(this._imageData);
        }*/
    }

    private sendSetImageEventHeader(imageSize: number) {
        var arrlen = 15;
        var buffer = new ArrayBuffer(arrlen);
        var uint8 = new Uint8Array(buffer);
        uint8.set(strToBytes("dsim"));
        var vId = new DataView(buffer);
        vId.setUint16(4, this._id, true);
        vId.setUint8(6, this._imageType);
        vId.setUint8(7, this._scaleType);
        vId.setUint8(8, this._bgR);
        vId.setUint8(9, this._bgG);
        vId.setUint8(10, this._bgB);

        vId.setUint32(11, imageSize, true);
        this.sendEvent(buffer);
    }

    set image(imageFilePath: string) {
        console.log("set image", imageFilePath);
        if (this._imageFilePath == imageFilePath)
            return;
        if (!fs.existsSync(imageFilePath)) {
            console.error("Image file not existis!");
            return;
        }

        //this._imageData = null;

        this._imageFilePath = imageFilePath;

        const stats = fs.statSync(this._imageFilePath);
        const fileSizeInBytes = stats.size;
        /*
                var arrlen = 15;
                var buffer = new ArrayBuffer(arrlen);
                var uint8 = new Uint8Array(buffer);
                uint8.set(strToBytes("dsim"));
                var vId = new DataView(buffer);
                vId.setUint16(4, this._id, true);
                vId.setUint8(6, this._imageType);
                vId.setUint8(7, this._scaleType);
                vId.setUint8(8, this._bgR);
                vId.setUint8(9, this._bgG);
                vId.setUint8(10, this._bgB);
        
                vId.setUint32(11, fileSizeInBytes, true);
                this.sendEvent(buffer);*/
        this.sendSetImageEventHeader(fileSizeInBytes);
        this.sendImage();
    }

    set imageUrl(imageUrl: string) {
        this.downloadImage(imageUrl, (imageData) => {
            if (imageData == null) {
                console.log("imageData = null!")
                return;
            }
            this._imageFilePath = null;
            this._imageUrl = imageUrl;
            this.sendSetImageEventHeader(imageData.byteLength);
            wstreamWrite.write(imageData);
        });

        /* this._imageUrl = imageUrl;
         var request = require('request').defaults({ encoding: null });
         request.get(s3Url, function (err, res, body) {
             //process exif here
         });*/
    }

    protected getCreateBytes() {
        let header = this.getCreateBytesHeader("dimg");

        const stats = fs.statSync(this._imageFilePath);
        const fileSizeInBytes = stats.size;


        var buffer = new ArrayBuffer(header.length + 9);
        var uint8 = new Uint8Array(buffer);
        uint8.set(header);
        var vId = new DataView(buffer);

        vId.setUint8(17, this._imageType);
        vId.setUint8(18, this._scaleType);
        vId.setUint8(19, this._bgR);
        vId.setUint8(20, this._bgG);
        vId.setUint8(21, this._bgB);

        vId.setUint32(22, fileSizeInBytes, true);

        return buffer;
    }

    /*
    var image = fs.readFile('targetImage.png', function(err, data) {
        fs.writeFile('outputImage.png', data, 'binary', function (err) {
            if (err) {
                console.log("There was an error writing the image")
            }
     
            else {
                console.log("There file was written")
            }
        });
    });
    */

}

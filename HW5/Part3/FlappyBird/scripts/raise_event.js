//death for raising character death event
function raiseEvent(){
    var toggle_event_string = "death";
    character.toggleEvent = toggle_event_string ;
    print("Raising Event Through Script");
}

raiseEvent();
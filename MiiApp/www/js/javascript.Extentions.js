//Helper function, to make boolean text tranlations work
Boolean.fromString = function(value,defaultValue){
	 try{
	 if (typeof value == 'boolean') return value;
	 if (value==null || value=="" || typeof value=='undefined') return defaultValue;
   switch(value.toLowerCase().trim()){
     case "true": case "yes": case "1": return true;
     case "false": case "no": case "0": case null: return false;
     default: return defaultValue;
   }
	 }catch (ex){
		//console.warn("Boolean ",ex,typeof value);
		return defaultValue;}
 };

//Helper function, to check if a Array contains a other object 
Array.prototype.contains = function contains(obj) {
    for (var i = 0; i < this.length; i++) {
        if (this[i] === obj) {
            return true;
        }
    }
    return false;
};
